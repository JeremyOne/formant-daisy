// Turns lyric text into the FM-1 engine's lyric data (see src/daisy_fx.h), using
// the plugin's own text-to-phoneme code (CMU dictionary, word definitions,
// acronyms, rests). The device never needs the dictionary.
//
//   fm1_encode --syx out.syx  "lyrics" | @file.txt   a SysEx message to send to the FM-1
//   fm1_encode --c NAME out.h "lyrics" | @file.txt   a C array (for a built-in lyric)
//   fm1_encode --show "lyrics"                        prints the syllables
//
// SysEx (7D = non-commercial ID, 44 46 = "DF" for Daisy), sent as consecutive messages
// because the FM-1's SysEx frames hold 640 bytes:
//   F0 7D 44 46 01 F7            begin
//   F0 7D 44 46 02 <bytes> F7    up to 512 lyric bytes each
//   F0 7D 44 46 03 F7            end (the FM-1 checks it, then sings it)
// Every lyric byte is below 0x80, as SysEx needs.
//
// Build (from the repository root):
//   g++ -std=c++17 -O2 -ISource ports/fm1/tools/fm1_encode.cpp Source/engine/*.cpp -o fm1_encode
#include "engine/Phonemes.h"
#include "engine/TextToPhonemes.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
const char* kPhones[] = { "iy", "ih", "eh", "ae", "aa", "ao", "ah", "uh", "uw", "er", "ax", "ey", "ay", "oy",
                          "aw", "ow", "p",  "b",  "t",  "d",  "k",  "g",  "f",  "v",  "th", "dh", "s",  "z",
                          "sh", "zh", "ch", "jh", "hh", "m",  "n",  "ng", "l",  "r",  "w",  "y" };

int code (int phonemeIndex)
{
    const std::string n = fd::phoneme (phonemeIndex).name;
    for (int i = 0; i < 40; ++i)
        if (n == kPhones[i]) return i + 1;
    return 0;
}

std::string readText (const std::string& arg)
{
    if (! arg.empty() && arg[0] == '@')
    {
        std::ifstream f (arg.substr (1));
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
    return arg;
}
} // namespace

std::vector<unsigned char> encode (const std::string& text)
{
    const auto L = fd::parseLyrics (text);
    std::vector<unsigned char> out;
    long lastRecord = -1;
    for (const auto& s : L.syllables)
    {
        if (s.rest && ! s.noteRest)
        {
            if (s.lineBreak && lastRecord >= 0) out[(size_t) lastRecord] |= 2; // a line break follows
            continue;
        }
        lastRecord = (long) out.size();
        if (s.rest)
        {
            out.insert (out.end(), { (unsigned char) (1 | (s.lineBreak ? 2 : 0)), 0, 0, 0 });
            continue;
        }
        out.push_back (0);
        out.push_back ((unsigned char) std::min<size_t> (s.onset.size(), 6));
        for (size_t i = 0; i < s.onset.size() && i < 6; ++i) out.push_back ((unsigned char) code (s.onset[i]));
        out.push_back ((unsigned char) code (s.nucleus));
        out.push_back ((unsigned char) std::min<size_t> (s.coda.size(), 6));
        for (size_t i = 0; i < s.coda.size() && i < 6; ++i) out.push_back ((unsigned char) code (s.coda[i]));
    }
    return out;
}

#ifndef FM1_ENCODE_NO_MAIN
int main (int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf (stderr, "usage: fm1_encode --syx out.syx TEXT | --c NAME out.h TEXT | --show TEXT  (TEXT or @file)\n");
        return 1;
    }
    const std::string mode = argv[1];
    if (mode == "--show")
    {
        const auto data = encode (readText (argv[2]));
        size_t i = 0;
        int n = 0;
        while (i < data.size())
        {
            const int flags = data[i], no = data[i + 1];
            std::printf ("%3d %s%s ", n++, flags & 1 ? "REST" : "", flags & 2 ? "/" : "");
            for (int k = 0; k < no; ++k) std::printf ("%s ", kPhones[data[i + 2 + k] - 1]);
            const int nuc = data[i + 2 + no], nc = data[i + 3 + no];
            if (nuc) std::printf ("[%s] ", kPhones[nuc - 1]);
            for (int k = 0; k < nc; ++k) std::printf ("%s ", kPhones[data[i + 4 + no + k] - 1]);
            std::printf ("\n");
            i += (size_t) (4 + no + nc);
        }
        std::printf ("%zu bytes\n", data.size());
        return 0;
    }
    if (mode == "--syx" && argc >= 4)
    {
        const auto data = encode (readText (argv[3]));
        if (data.size() > 4096) { std::fprintf (stderr, "lyric too long: %zu bytes (max 4096)\n", data.size()); return 1; }
        std::FILE* f = std::fopen (argv[2], "wb");
        if (! f) return 1;
        auto msg = [f] (unsigned char cmd, const unsigned char* p, size_t n) {
            const unsigned char head[] = { 0xF0, 0x7D, 0x44, 0x46, cmd };
            std::fwrite (head, 1, sizeof head, f);
            if (n) std::fwrite (p, 1, n, f);
            std::fputc (0xF7, f);
        };
        msg (0x01, nullptr, 0);
        size_t chunks = 0;
        for (size_t i = 0; i < data.size(); i += 512, ++chunks)
            msg (0x02, data.data() + i, std::min<size_t> (512, data.size() - i));
        msg (0x03, nullptr, 0);
        std::fclose (f);
        std::printf ("wrote %s (%zu lyric bytes in %zu messages)\n", argv[2], data.size(), chunks + 2);
        return 0;
    }
    if (mode == "--c" && argc >= 5)
    {
        const auto data = encode (readText (argv[4]));
        std::FILE* f = std::fopen (argv[3], "w");
        if (! f) return 1;
        std::fprintf (f, "/* Generated by ports/fm1/tools/fm1_encode. */\nstatic const unsigned char %s[%zu] = {", argv[2], data.size());
        for (size_t i = 0; i < data.size(); ++i) std::fprintf (f, "%s%u,", i % 20 == 0 ? "\n    " : " ", data[i]);
        std::fprintf (f, "\n};\n");
        std::fclose (f);
        std::printf ("wrote %s (%zu bytes)\n", argv[3], data.size());
        return 0;
    }
    std::fprintf (stderr, "bad arguments\n");
    return 1;
}
#endif
