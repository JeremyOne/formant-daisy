// Offline test: sings the Daisy Bell chorus to a WAV file without a DAW.
//   render_daisy out.wav [bpm] [source 0..1] ["custom lyrics"]
// Uses the same engine as the plugin, driven by the same note list as
// tools/make_daisy_midi.py.
#include "engine/FormantSinger.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct N
{
    int note;
    float beats;
};

// Chorus melody (F major, 3/4). One entry per sung syllable.
// clang-format off
static const N kMelody[] = {
    // Dai-sy, Dai-sy, give me your an-swer, do
    {72,3},{69,3},{65,3},{60,3},{62,1},{64,1},{65,1},{62,2},{65,1},{60,6},
    // I'm half cra-zy, all for the love of you
    {67,3},{72,3},{69,3},{65,3},{62,1},{64,1},{65,1},{67,2},{69,1},{67,6},
    // It won't be a sty-lish mar-riage
    {69,1},{70,1},{69,1},{67,1},{72,2},{69,1},{67,2},{65,3},
    // I can't af-ford a car-riage
    {67,1},{69,2},{65,1},{62,2},{65,1},{62,2},{60,3},
    // But you'll look sweet up-on the seat of a bi-cy-cle built for two
    {60,1},{65,2},{69,1},{67,2},{60,1},{65,2},{69,1},{67,2},{69,1},{70,1},{72,1},{69,1},{65,1},{67,2},{60,1},{65,6},
};
// clang-format on

static const char* kLyrics =
    "Daisy Daisy give me your answer do\n"
    "I'm half crazy all for the love of you\n"
    "It won't be a stylish marriage\n"
    "I can't afford a carriage\n"
    "But you'll look sweet upon the seat\n"
    "Of a bicycle built for two";

static void writeWav (const char* path, const std::vector<float>& s, int sr)
{
    FILE* f = std::fopen (path, "wb");
    if (! f) { std::perror ("open"); std::exit (1); }
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    const uint32_t dataBytes = (uint32_t) s.size() * 2;
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); u32 (16); u16 (1); u16 (1); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 2); u16 (2); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (float x : s)
    {
        float c = x < -1 ? -1 : (x > 1 ? 1 : x);
        int16_t v = (int16_t) (c * 32767.0f);
        std::fwrite (&v, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const char* out = argc > 1 ? argv[1] : "daisy.wav";
    const double bpm = argc > 2 ? std::atof (argv[2]) : 150.0;
    const float source = argc > 3 ? (float) std::atof (argv[3]) : 0.5f;
    const std::string text = argc > 4 ? argv[4] : kLyrics;
    const int sr = 48000;

    auto lyrics = std::make_shared<fd::Lyrics> (fd::parseLyrics (text));
    std::printf ("syllables to sing: %d, notes: %d\n", lyrics->singableCount(), (int) (sizeof (kMelody) / sizeof (N)));

    fd::FormantSinger singer;
    singer.prepare (sr);
    fd::SingerParams p;
    p.source = source;
    p.gain = 0.6f;
    singer.setParams (p);
    singer.setLeadSamples (sr * 60 / 1000);
    singer.setLyrics (lyrics);

    // build note events in samples
    struct Ev { int64_t t; bool on; int note; };
    std::vector<Ev> evs;
    const double spb = sr * 60.0 / bpm;
    double beat = 0;
    for (auto& n : kMelody)
    {
        const int64_t on = (int64_t) (beat * spb);
        const int64_t off = (int64_t) ((beat + n.beats * 0.92) * spb);
        evs.push_back ({ on, true, n.note });
        evs.push_back ({ off, false, n.note });
        beat += n.beats;
    }
    const int64_t total = (int64_t) ((beat + 2) * spb);

    std::vector<float> audio ((size_t) total, 0.0f);
    const int block = 256;
    size_t ei = 0;
    std::sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && ! a.on && b.on); });
    for (int64_t pos = 0; pos < total; pos += block)
    {
        const int n = (int) std::min<int64_t> (block, total - pos);
        while (ei < evs.size() && evs[ei].t < pos + n)
        {
            const int off = (int) (evs[ei].t - pos);
            if (evs[ei].on) singer.noteOn (evs[ei].note, 0.8f, off);
            else singer.noteOff (evs[ei].note, off);
            ++ei;
        }
        singer.process (audio.data() + pos, n);
    }
    float peak = 0;
    double rms = 0;
    for (float x : audio) { peak = std::max (peak, std::abs (x)); rms += x * x; }
    std::printf ("peak %.3f  rms %.4f  seconds %.1f\n", peak, std::sqrt (rms / audio.size()), audio.size() / (double) sr);
    writeWav (out, audio, sr);
    return 0;
}
