// English text -> syllables of phonemes.
//
// Pipeline: tokenize -> (dictionary | letter-to-sound rules) -> syllabify.
// Letter-to-sound rules follow the structure of the classic NRL rule set
// (Elovitz, Johnson, McHugh & Shore, 1976), the same family of rules early
// text-to-speech systems such as DECtalk grew out of.
//
// Text syntax supported:
//   plain words             daisy daisy give me your answer do
//   hyphen syllable hints   Dai-sy    (forces where syllables split)
//   punctuation rests       , ; :  = 1 step rest,  . ! ? and newlines = 2 steps
//   raw phonemes            [d ey | z iy]   or DECtalk style [dey ziy]
//   bar markers             -bar.12-  on its own line: what follows starts at bar 12
//   word definitions        ceiling = s iy | l ih ng  on its own line
//                           ('|' or '-' splits syllables inside brackets)
#pragma once
#include <string>
#include <vector>

namespace fd
{
struct Syllable
{
    std::vector<int> onset;
    int nucleus = -1;
    std::vector<int> coda;
    std::string label; // spelling chunk shown in the UI
    int word = -1;     // index into Lyrics::words
    int line = 0;      // which line of the lyrics text it came from
    bool rest = false;     // a pause from punctuation or a line break
    int restSteps = 0;     // Tempo mode: steps of silence
    bool noteRest = false; // Notes mode: punctuation consumes one MIDI note silently
                           // (line breaks alone don't)
    bool lineBreak = false; // a newline follows here: singing waits for the next bar
};

// A bar marker ("-bar.12-") pins the lyrics after it to a bar of the song.
struct Section
{
    long long bar = 0; // 0-based bar index (bar 1 = 0)
    int firstSyl = 0;  // first syllable of the section
};

struct Lyrics
{
    std::vector<Syllable> syllables;
    std::vector<std::string> words;
    // Empty when the lyrics have no bar markers. Otherwise sorted by bar and by
    // syllable; text before the first marker becomes a section at bar 1.
    std::vector<Section> sections;
    int singableCount() const;
    int noteLength() const; // notes needed for one pass in Notes mode (syllables + punctuation rests)
    // Steps for one pass: Notes mode = 1 per syllable or punctuation; Tempo mode
    // counts each punctuation's rest steps.
    int stepLength (bool tempoMode) const;
    // Bars for one pass when each step is 1/stepsPerBar of a bar. Every line
    // starts on a new bar, so each line is rounded up to whole bars.
    int barLength (double stepsPerBar, bool tempoMode) const;
};

// spellAcronyms: all-caps words of 2-5 letters (PHP, NASA) are read letter by letter.
Lyrics parseLyrics (const std::string& text, bool spellAcronyms = true);

// Phonemes for one word (dictionary or rules), flat, for debugging/tests.
std::vector<int> wordToPhonemes (const std::string& word);
std::string phonemesToString (const std::vector<int>& phs);
} // namespace fd
