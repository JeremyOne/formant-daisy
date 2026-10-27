// Prints how each word of a lyric file is sung: syllable labels and phonemes.
// Build: g++ -std=c++17 -O1 -ISource tools/pron_check.cpp Source/engine/*.cpp -o pron_check
#include "engine/TextToPhonemes.h"
#include "engine/Phonemes.h"
#include <fstream>
#include <iostream>
#include <sstream>

int main (int argc, char** argv)
{
    std::stringstream ss;
    if (argc > 1) { std::ifstream f (argv[1]); ss << f.rdbuf(); }
    else ss << std::cin.rdbuf();
    auto L = fd::parseLyrics (ss.str());
    int lastWord = -2;
    std::string line;
    for (auto& s : L.syllables)
    {
        if (s.rest) { if (s.lineBreak) { std::cout << line << "\n"; line.clear(); } continue; }
        std::string ph;
        for (int id : s.onset) ph += fd::phoneme (id).name, ph += ' ';
        ph += fd::phoneme (s.nucleus).name;
        for (int id : s.coda) ph += ' ', ph += fd::phoneme (id).name;
        if (s.word != lastWord) line += (line.empty() ? "" : "  ") + L.words[(size_t) s.word] + ":";
        else line += " |";
        line += " " + s.label + "(" + ph + ")";
        lastWord = s.word;
    }
    std::cout << line << "\n" << "notes: " << L.noteLength() << "\n";
}
