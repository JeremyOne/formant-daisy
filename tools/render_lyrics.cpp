// Renders lyrics as a run of equal notes (Notes mode), for listening tests.
//   render_lyrics out.wav "lyrics" [bpm] [beatsPerNote] [gate 0..1] [passes]
// The lyrics can also be read from a file: render_lyrics out.wav @file.txt ...
#include "engine/FormantSinger.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
int main (int argc, char** argv)
{
    const int sr = 48000;
    const char* out = argc > 1 ? argv[1] : "lyrics.wav";
    std::string text = argc > 2 ? argv[2] : "Project Mammoth Is the best.";
    if (! text.empty() && text[0] == '@')
    {
        std::string body;
        if (FILE* in = std::fopen (text.c_str() + 1, "rb")) { char buf[4096]; size_t n; while ((n = std::fread (buf, 1, sizeof buf, in)) > 0) body.append (buf, n); std::fclose (in); }
        text = body;
    }
    const int passes = argc > 6 ? std::atoi (argv[6]) : 3;
    double bpm = argc > 3 ? std::atof (argv[3]) : 120, beats = argc > 4 ? std::atof (argv[4]) : 0.5, gate = argc > 5 ? std::atof (argv[5]) : 0.85;
    auto L = std::make_shared<fd::Lyrics> (fd::parseLyrics (text));
    fd::FormantSinger s; s.prepare (sr); fd::SingerParams p; p.gain = 0.6f; s.setParams (p); s.setLeadSamples (sr * 60 / 1000); s.setLyrics (L);
    const int notes[] = { 60, 62, 64, 65, 67, 65, 64, 62 };
    const double spb = sr * 60.0 / bpm; const int count = L->noteLength() * passes;
    std::vector<float> a ((size_t) ((count * beats + 1) * spb));
    for (int k = 0; k < count; ++k)
    {
        size_t on = (size_t) (k * beats * spb), off = (size_t) ((k + gate) * beats * spb), nx = (size_t) ((k + 1) * beats * spb);
        s.noteOn (notes[k % 8], 0.8f, 0); s.process (a.data() + on, (int) (off - on));
        s.noteOff (notes[k % 8], 0); s.process (a.data() + off, (int) (nx - off));
    }
    FILE* f = std::fopen (out, "wb"); auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + (uint32_t) a.size() * 2); std::fwrite ("WAVEfmt ", 1, 8, f); u32 (16); u16 (1); u16 (1); u32 (sr); u32 (sr * 2); u16 (2); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 ((uint32_t) a.size() * 2);
    for (float v : a) { int16_t q = (int16_t) (std::clamp (v, -1.0f, 1.0f) * 32767); std::fwrite (&q, 2, 1, f); }
    std::fclose (f);
}
