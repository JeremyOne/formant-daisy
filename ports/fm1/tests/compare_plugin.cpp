// Plays the same notes through the plugin's engine (float) and the FM-1 port
// (fixed point) and compares them: the F1 / F2 tracks the two engines make,
// their loudness over time, and the overall level. Also writes both as WAVs.
//
// Build (from the repository root):
//   g++ -std=c++17 -O2 -ISource -Iports/fm1/src -x c ports/fm1/src/daisy_fx.c -x c++ \
//       ports/fm1/tests/compare_plugin.cpp Source/engine/*.cpp -o compare_plugin
//   ./compare_plugin [outDir]
#include "engine/FormantSinger.h"
#include "engine/TextToPhonemes.h"
extern "C" {
#include "daisy_fx.h"
}
#define FM1_ENCODE_NO_MAIN
#include "../tools/fm1_encode.cpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr int kFs = 44100;

struct Note { int pitch; double start, len; }; // beats

// "Daisy Bell" (Harry Dacre, 1892, public domain): the first two lines of the chorus
// in 4/4 at 120 BPM, as in the DnB demo
const std::vector<Note> kMelody = {
    { 72, 0, 2 }, { 68, 2, 2 }, { 65, 4, 2 }, { 60, 6, 2 }, { 61, 8, 1 }, { 63, 9, 1 }, { 65, 10, 1 }, { 61, 11, 1 },
    { 65, 12, 1 }, { 60, 13, 3 }, { 67, 16, 2 }, { 72, 18, 2 }, { 68, 20, 2 }, { 65, 22, 2 }, { 61, 24, 1 },
    { 63, 25, 1 }, { 65, 26, 1 }, { 67, 27, 1 }, { 68, 28, 1 }, { 67, 29, 3 },
};
const char* kText = "Daisy Daisy give me your answer do\nI'm half crazy all for the love of you\n";

void writeWav (const std::string& path, const std::vector<float>& a)
{
    std::FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + (uint32_t) a.size() * 2); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (1); u32 (kFs); u32 (kFs * 2); u16 (2); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 ((uint32_t) a.size() * 2);
    for (float v : a) { int16_t q = (int16_t) std::lround (std::clamp (v, -1.0f, 1.0f) * 32767); std::fwrite (&q, 2, 1, f); }
    std::fclose (f);
}

struct Track { std::vector<float> audio, f1, f2, voiced; };

template <typename NoteOn, typename NoteOff, typename Render, typename Probe>
Track run (NoteOn on, NoteOff off, Render render, Probe probe)
{
    const double spb = kFs * 60.0 / 120.0;
    struct Ev { long t; bool on; int note; };
    std::vector<Ev> ev;
    for (auto& n : kMelody)
    {
        ev.push_back ({ (long) (n.start * spb), true, n.pitch });
        ev.push_back ({ (long) ((n.start + n.len * 0.92) * spb), false, n.pitch });
    }
    std::stable_sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t; });
    const long total = (long) (33 * spb);
    Track tr;
    tr.audio.assign ((size_t) total, 0.0f);
    size_t e = 0;
    for (long t = 0; t < total; t += 16)
    {
        while (e < ev.size() && ev[e].t <= t)
        {
            if (ev[e].on) on (ev[e].note); else off (ev[e].note);
            ++e;
        }
        render (tr.audio.data() + t, (int) std::min<long> (16, total - t));
        float f1, f2, v;
        probe (f1, f2, v);
        tr.f1.push_back (f1); tr.f2.push_back (f2); tr.voiced.push_back (v);
    }
    return tr;
}
} // namespace

int main (int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    // the plugin's engine, no lead (the device has none) and no vibrato jitter difference on purpose
    fd::FormantSinger plug;
    plug.prepare (kFs);
    fd::SingerParams sp;
    sp.gain = 0.7f;
    plug.setParams (sp);
    plug.setLeadSamples (0);
    plug.setLyrics (std::make_shared<fd::Lyrics> (fd::parseLyrics (kText)));
    Track a = run ([&] (int n) { plug.noteOn (n, 0.8f, 0); }, [&] (int n) { plug.noteOff (n, 0); },
                   [&] (float* o, int n) { plug.process (o, n); },
                   [&] (float& f1, float& f2, float& v) { f1 = plug.vizF1.load(); f2 = plug.vizF2.load(); v = plug.vizVoicing.load(); });

    // the FM-1 engine
    static dfx_singer_t s;
    dfx_init (&s);
    const auto data = encode (kText);
    const int nsyl = dfx_set_lyrics (&s, data.data(), (uint32_t) data.size());
    std::vector<int32_t> buf (16);
    Track b = run ([&] (int n) { dfx_note_on (&s, (uint8_t) n, 102); }, [&] (int n) { dfx_note_off (&s, (uint8_t) n); },
                   [&] (float* o, int n) {
                       std::fill (buf.begin(), buf.end(), 0);
                       dfx_render (&s, buf.data(), (uint32_t) n);
                       for (int i = 0; i < n; ++i) o[i] = (float) buf[(size_t) i] / 32768.0f;
                   },
                   [&] (float& f1, float& f2, float& v) { f1 = (float) dfx_f1 (&s); f2 = (float) dfx_f2 (&s); v = (float) s.cur.av / 32768.0f; });

    writeWav (dir + "/plugin.wav", a.audio);
    writeWav (dir + "/fm1.wav", b.audio);

    // formant tracks while both are clearly voicing
    std::vector<float> d1, d2;
    for (size_t i = 0; i < a.f1.size(); ++i)
        if (a.voiced[i] > 0.3f && b.voiced[i] > 0.3f)
        {
            d1.push_back (std::abs (a.f1[i] - b.f1[i]));
            d2.push_back (std::abs (a.f2[i] - b.f2[i]));
        }
    auto pct = [] (std::vector<float> v, double q) { std::sort (v.begin(), v.end()); return v.empty() ? 0.0f : v[(size_t) (q * (v.size() - 1))]; };

    // loudness over time (50 ms frames) and overall
    std::vector<double> ra, rb;
    double ea = 0, eb = 0;
    const size_t fr = kFs / 20;
    for (size_t i = 0; i + fr <= a.audio.size(); i += fr)
    {
        double sa = 0, sb = 0;
        for (size_t k = i; k < i + fr; ++k) { sa += a.audio[k] * a.audio[k]; sb += b.audio[k] * b.audio[k]; }
        ra.push_back (std::sqrt (sa / fr)); rb.push_back (std::sqrt (sb / fr));
        ea += sa; eb += sb;
    }
    double ma = 0, mb = 0;
    for (size_t i = 0; i < ra.size(); ++i) { ma += ra[i]; mb += rb[i]; }
    ma /= ra.size(); mb /= rb.size();
    double num = 0, da = 0, db = 0;
    for (size_t i = 0; i < ra.size(); ++i) { num += (ra[i] - ma) * (rb[i] - mb); da += (ra[i] - ma) * (ra[i] - ma); db += (rb[i] - mb) * (rb[i] - mb); }
    const double corr = num / std::sqrt (da * db + 1e-12);
    const double levelDb = 10 * std::log10 ((eb + 1e-12) / (ea + 1e-12));
    float peak = 0;
    for (float v : b.audio) peak = std::max (peak, std::abs (v));

    std::printf ("syllables: %d (%zu lyric bytes)\n", nsyl, data.size());
    std::printf ("F1 difference: median %.0f Hz, 90%% %.0f Hz  (%zu voiced ticks)\n", pct (d1, 0.5), pct (d1, 0.9), d1.size());
    std::printf ("F2 difference: median %.0f Hz, 90%% %.0f Hz\n", pct (d2, 0.5), pct (d2, 0.9));
    std::printf ("loudness envelope correlation: %.3f\n", corr);
    std::printf ("level, FM-1 vs plugin: %+.1f dB   FM-1 peak %.2f\n", levelDb, peak);
    const bool ok = nsyl == 20 && pct (d1, 0.5) < 25 && pct (d2, 0.5) < 60 && corr > 0.8 && std::abs (levelDb) < 4 && peak < 1.0f;
    std::printf (ok ? "OK\n" : "CHECK FAILED\n");
    return ok ? 0 : 1;
}
