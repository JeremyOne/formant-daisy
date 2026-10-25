// Renders each phoneme group held at a low pitch, for formant/level checks.
#include "engine/FormantSinger.h"
#include <cstdio>
#include <vector>
#include <string>
#include <memory>
int main (int argc, char** argv)
{
    const int sr = 48000;
    std::string text = argc > 1 ? argv[1] : "[iy] [ih] [eh] [ae] [aa] [ao] [ah] [uh] [uw] [er] [ax]";
    float src = argc > 2 ? std::atof (argv[2]) : 0.5f;
    int note = argc > 3 ? std::atoi (argv[3]) : 45;
    auto lyr = std::make_shared<fd::Lyrics> (fd::parseLyrics (text));
    fd::FormantSinger s; s.prepare (sr);
    fd::SingerParams p; p.source = src; p.vibDepthCents = 0; p.gain = 0.6f; p.breath = 0; s.setParams (p); s.setLyrics (lyr);
    const int n = lyr->singableCount();
    std::vector<float> out ((size_t) (n * sr), 0.f);
    for (int k = 0; k < n; ++k)
    {
        float* b = out.data() + (size_t) k * sr;
        s.noteOn (note, 0.8f, 0);
        s.process (b, sr * 3 / 4);
        s.noteOff (note, 0);
        s.process (b + sr * 3 / 4, sr / 4);
    }
    FILE* f = std::fopen ("build-tools/vowels.raw", "wb"); std::fwrite (out.data(), 4, out.size(), f); std::fclose (f);
    std::printf ("%d\n", n);
}
