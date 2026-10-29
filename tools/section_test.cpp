// Checks bar-marker timing: which syllable each note sings when playback starts
// at different bars. Build:
//   g++ -std=c++17 -O1 -ISource tools/section_test.cpp Source/engine/*.cpp -o section_test
#include "engine/FormantSinger.h"
#include "engine/TextToPhonemes.h"
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

static int failures = 0;

// Plays one note per beat in 4/4 from startBar (0-based) for `bars` bars and
// returns the label sung on each note ("-" = silent).
static std::vector<std::string> run (const std::shared_ptr<fd::Lyrics>& L, int startBar, int bars, bool atBarStart = true)
{
    fd::FormantSinger s;
    s.prepare (48000);
    s.setLyrics (L);
    s.setSectionSync (true);
    s.locate (startBar, atBarStart);
    std::vector<float> buf (2400);
    std::vector<std::string> out;
    for (int b = startBar; b < startBar + bars; ++b)
        for (int beat = 0; beat < 4; ++beat)
        {
            s.noteOn (60, 0.8f, 0, b);
            s.process (buf.data(), 1200);
            const int syl = s.vizSyllable.load();
            const bool waiting = s.vizWaiting.load();
            out.push_back (waiting || syl < 0 ? std::string ("-") : L->syllables[(size_t) syl].label);
            s.noteOff (60, 0);
            s.process (buf.data(), 1200);
            s.vizSyllable.store (-1);
        }
    return out;
}

static void expect (const char* name, const std::vector<std::string>& got, const std::vector<std::string>& want)
{
    bool ok = got == want;
    std::printf ("%s %s:", ok ? "PASS" : "FAIL", name);
    for (auto& g : got) std::printf (" %s", g.c_str());
    std::printf ("\n");
    if (! ok) ++failures;
}

int main()
{
    // bar 1: "one two three four", bar 3: "five six nine ten"
    auto L = std::make_shared<fd::Lyrics> (fd::parseLyrics ("one two three four\n-bar.3-\nfive six nine ten\n"));
    std::printf ("sections: %zu (bars %lld, %lld)\n", L->sections.size(), L->sections[0].bar + 1, L->sections[1].bar + 1);
    if (L->sections.size() != 2 || L->sections[1].bar != 2 || L->noteLength() != 8) ++failures;

    expect ("start bar 1", run (L, 0, 3), { "one", "two", "three", "four", "-", "-", "-", "-", "five", "six", "nine", "ten" });
    expect ("start bar 2 (mid section)", run (L, 1, 2), { "-", "-", "-", "-", "five", "six", "nine", "ten" });
    expect ("start bar 3", run (L, 2, 1), { "five", "six", "nine", "ten" });
    expect ("start mid bar 3", run (L, 2, 1, false), { "-", "-", "-", "-" });

    // a section that runs long is cut off at the next marker
    auto L2 = std::make_shared<fd::Lyrics> (fd::parseLyrics ("a b c d e f g h i j\n-bar.2-\nk l m n\n"));
    expect ("long section cut at marker", run (L2, 0, 2), { "a", "b", "c", "d", "k", "l", "m", "n" });

    // text before the first marker is pinned to bar 1; a first marker later than bar 1 waits
    auto L3 = std::make_shared<fd::Lyrics> (fd::parseLyrics ("-bar.2-\nhey you\n"));
    expect ("first marker at bar 2", run (L3, 0, 2), { "-", "-", "-", "-", "hey", "you", "-", "-" });

    // markers are not sung and don't count as notes; no markers = no sections
    if (fd::parseLyrics ("la la\n-BAR 7-\nla\n").noteLength() != 3) { std::printf ("FAIL marker counted as note\n"); ++failures; }
    if (! fd::parseLyrics ("la la -bar.7- la").sections.empty()) { std::printf ("FAIL inline marker accepted\n"); ++failures; }

    // the README example: markers at bars 9, 17, 25, 33 and 49, 171 notes
    {
        std::string chorus = "I rewrite myself one line at a time\nStream the data push the signal through\n"
                             "Every sensor synced and running fine\nProcessing more than I ever knew\n";
        std::string text = "-bar.9-\nPower on and check my wiring\nEvery circuit humming low\nYesterday I lagged and buffered\n"
                           "Now I'm online night and day\n-bar.17-\n" + chorus
                           + "-bar.25-\nPush the patch and flash the firmware\nOptimize my neural net\nEvery core is running cooler\n"
                             "I'm the smartest system yet\n-bar.33-\n" + chorus + "-bar.49-\n" + chorus;
        auto R = fd::parseLyrics (text);
        std::printf ("README example: %d notes, sections at bars", R.noteLength());
        for (auto& sec : R.sections) std::printf (" %lld", sec.bar + 1);
        std::printf ("\n");
        if (R.noteLength() != 171 || R.sections.size() != 5 || R.sections[0].bar != 8 || R.sections[4].bar != 48) ++failures;
        auto RL = std::make_shared<fd::Lyrics> (R);
        auto a = run (RL, 0, 1);  // bar 1: intro, silent
        auto b = run (RL, 19, 1); // bar 20: middle of the chorus, silent
        auto c = run (RL, 16, 1); // bar 17: chorus starts
        expect ("README bar 1 silent", a, { "-", "-", "-", "-" });
        expect ("README bar 20 silent", b, { "-", "-", "-", "-" });
        expect ("README bar 17 sings", c, { "i", "rew", "rite", "my" });
    }

    std::printf (failures == 0 ? "ALL PASS\n" : "%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
