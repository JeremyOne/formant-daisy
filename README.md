# Formant Daisy

A singing robot instrument plugin (VST3 / Standalone). Type lyrics, play MIDI, and it sings them one syllable per note using formant synthesis, the technique behind the 1961 Bell Labs IBM 7094 "Daisy Bell" demo and DECtalk.

![Formant Daisy, full view](docs/images/full-view.png)

**Version:** 0.14.0. See [CHANGELOG.md](CHANGELOG.md).

## Contents

- [Install](#install)
- [Quick start](#quick-start)
- [The display](#the-display)
- [Writing lyrics](#writing-lyrics)
- [Notes mode and Tempo mode](#notes-mode-and-tempo-mode)
- [Controls](#controls)
- [Audio input (vocoder style)](#audio-input-vocoder-style)
- [Compact 9:16 view, colors and robots](#compact-916-view-colors-and-robots)
- [How the sound is made](#how-the-sound-is-made)
- [About the technologies](#about-the-technologies)
- [Building from source](#building-from-source)
- [Releases](#releases)

## Install

Download the zip for your system from the [Releases](../../releases) page.

| System | Copy `Formant Daisy.vst3` to |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` |
| Linux | `~/.vst3/` |

Rescan plugins in your DAW. Formant Daisy shows up as an instrument from **Project Mammoth**. The Standalone zip has a version you can run without a DAW.

## Quick start

1. Put Formant Daisy on a MIDI track. The Daisy Bell chorus is already loaded as lyrics.
2. Play notes. Each note sings the next syllable at that pitch, and holding the note holds the vowel.
3. For the full song, generate a MIDI file with `python3 tools/make_daisy_midi.py 150` and drop `DaisyBell_chorus.mid` on the track.
4. For the 1961 sound, set **VOICE** fully left (BUZZ) and **LO-FI** to about 60%.

## The display

| Area | What it shows |
|---|---|
| Top line (`>> DEY-ZIY ...`) | The phonemes of the current lyric line. Sung syllables are orange, the current one is cream, and upcoming ones are dim. |
| Big word | The word being sung, lit up syllable by syllable, with the next few words underneath and a `NOW SINGING x/y` counter. |
| Formant chart | Every phoneme placed by its first two formants (F1 down, F2 left). Vowels are teal, gliding vowels have a hollow marker, and consonants are pink. The glowing dot and its trail show the voice moving between them, and their color shows the third formant (F3): pink when it's low (R, ER), warm for normal vowels, blue when it's high (EE). Consonant hiss appears as static around the dot: tight for S, wide for F and TH, whiter for high hiss and pinker for low hiss. |
| BAR and beat boxes | Top-right of the chart. The current bar and beat from your DAW, with `WAIT` while a new line waits for the next bar. |
| Spectrum | The live frequency spectrum. Bars near F1, F2 and F3 are colored to match the key in the corner. |
| Scope | The output waveform. |
| Robot | The mouth follows the formants (F2 sets width, F1 sets height), the antenna lights while voiced, and the eyes blink on each rest. Click it to change the robot. |

The **?** button shows a quick reference in place of the chart:

![Help panel](docs/images/help.png)

## Writing lyrics

Type in the LYRICS box. Changes apply about a quarter second after you stop typing. The label above the box shows `LENGTH: n (m BARS)`, the notes needed for one pass and the bars that takes at the DIV note value in your project's time signature.

| You type | What happens |
|---|---|
| `daisy daisy` | Normal words. They're looked up in the [CMU Pronouncing Dictionary](https://github.com/cmusphinx/cmudict) (about 125,000 words, built in). Words it doesn't have are tried as a known word plus an ending (`slumbers`, `deathless`) or two known words joined (`webmail`). Anything else uses English letter-to-sound rules. |
| `ceiling = s iy \| l ih ng` | On a line of its own, sets how a word is said everywhere in the lyrics. The right side can be phonemes, `[phonemes]`, or a respelling in plain words (`taxicab = tacksee cab`). Definition lines aren't sung and don't count toward LENGTH. They're saved with the lyrics, so they stay with your project. |
| `sea-breakers`, `off-line` | Hyphenated compounds are read part by part. |
| `Dai-sy` | Hyphens force where the syllables split. |
| `[d ey \| z iy]` | Raw phonemes. `\|` splits syllables. DECtalk-style `[dey ziy]` also works. |
| `NSA`, `NASA` | All-caps words of 2 to 5 letters are spelled letter by letter ("en ess ay"), unless the dictionary says them as a word (NASA, CERT). Toggle with **ABC**. Single letters like `I` and `A` stay words. To make an acronym a word, define it: `CISA = s ih \| s ax`. |
| `,` `;` `:` `.` `!` `?` | Each mark is one rest. In Notes mode it uses up one note in silence, so `....` rests for four notes. |
| New line | Singing pauses until the next bar starts, then the next line begins on its downbeat. If a line already ends on the last beat, nothing is skipped. |
| Numbers | Read digit by digit. |

Phonemes: `iy ih eh ae aa ao ah uh uw er ax ey ay oy aw ow p b t d k g f v th dh s z sh zh ch jh hh m n ng l r w y`

![A line waiting for the next bar](docs/images/line-wait.png)

## Notes mode and Tempo mode

**Notes mode** (default): every MIDI note sings the next syllable at that note's pitch. Overlapping notes glide (GLIDE). Because the notes drive the syllables, the singing follows your DAW's tempo automatically.

**Tempo mode**: hold any note and it speaks one syllable per **DIV** step (1/4, 1/8, 1/8T, 1/16), locked to the host's bar and beat, at the held pitch. **GATE** sets how much of each step is voiced.

In both modes the plugin follows the host's bar, beat and time signature while it plays. When stopped, it keeps counting at the project tempo and the beat display dims.

**Restarting the lyrics:** the RESTART button, the automatable "Restart Lyrics" parameter, or automatically when the transport starts (**PLAY>1**).

## Controls

![Controls](docs/images/controls.png)

| Control | What it does |
|---|---|
| DAISY | Loads the Daisy Bell chorus as lyrics. |
| RESTART | Jumps back to the first syllable. |
| LOOP | Starts the lyrics over when they run out. |
| ? | Shows the help panel. |
| R | Resets every parameter to its default. Lyrics are kept. |
| MODE | Notes or Tempo mode. |
| DIV | Tempo mode step length. Also used for the bar count in LENGTH. |
| PLAY>1 | Restarts the lyrics when the DAW starts playing. |
| ABC | Spells out all-caps acronyms. |
| VOICE | The voice source, from a bright buzz (sawtooth) to a softer glottal pulse. |
| BREATH | Adds breathy noise to voiced sounds. |
| FORMANT | Scales every formant. Below 1 is a larger voice, above 1 a smaller one. |
| CONS LEN | Makes consonants shorter or longer. |
| GLIDE | Pitch slide between overlapping notes. |
| VIBRATO / VIB RATE | Vibrato depth (cents) and speed. It fades in after each note starts. |
| GATE | Tempo mode: how much of each step is voiced. |
| LEAD | Starts consonants early so vowels land exactly on the beat. The plugin reports this as latency so your DAW compensates. Set it to 0 for live playing. |
| LO-FI | Sample-rate and bit reduction. |
| VOLUME | Output level. |
| CARRIER | What the formants shape: AUTO, BUZZ or INPUT. See below. |
| 9:16 / FULL | Switches between the full and compact views. |
| COLOR | Cycles the color schemes. |

All settings, the lyrics, the view, color scheme and robot are saved with your project.

## Audio input (vocoder style)

Route audio into the plugin and the lyrics' vowels and consonants are shaped onto it, like a talk box. MIDI notes still trigger each syllable, the pitch comes from the incoming audio (a chord pad sings in chords), and consonant noise stays internal so words remain readable.

**In Ableton:** on the source track, set "Audio To" to the Formant Daisy track and choose the plugin in the second menu. You can also use the Sidechain section in the plugin's device header.

**CARRIER** sets the source:

- **AUTO** (default): uses the input whenever audio has arrived in the last 1.5 seconds, otherwise the internal buzz. The button turns orange while the input is in use.
- **BUZZ:** always the internal buzz.
- **INPUT:** always the input.

The input is leveled automatically, and the **IN** meter shows its level. Bright, harmonically rich sources (saw pads, distorted guitar, noise) work best. A pure sine has no harmonics for the formants to shape.

## Compact 9:16 view, colors and robots

Click **9:16** for a compact view with just the display and the robot. It's 540×960, exactly half of a 1080×1920 YouTube Shorts frame, so a 2× scale gives clean pixels. **FULL** switches back. Both buttons stay dim until you hover over them, so they barely show in recordings.

<img src="docs/images/compact-view.png" alt="Compact 9:16 view" width="270">

**COLOR** cycles ten color schemes: DAISY, AMBER (Amber Terminal), POCKET (Pocket Green), SYNTH (Synthwave), HAZY (Hazy Vaporwave), NEUTRAL (Neutral Palette 2), HAPPY (Happy Feet), TAM (Tam & Zpět), BEES (A House of Bees) and SPORTS (Sports Day).

![Color schemes](docs/images/color-schemes.png)

**Click the robot** to switch between 11 robots. They all share the same speaker-grille mouth and eyes.

![Robots](docs/images/robots.png)

## How the sound is made

A voice is a buzzy source (the vocal cords) going through a filter (the throat and mouth). The filter's resonant peaks are the **formants**, and the pattern of peaks is what makes each vowel.

- **F1** (about 250 to 900 Hz) follows how open the jaw is. EE and OO are low, AH is high.
- **F2** (about 600 to 2500 Hz) follows the tongue position and lip rounding. EE is high, OO is low.
- Consonants are noise through bandpass filters (S, SH, F), or a short silence then a burst of noise (P, T, K).

The engine is a Klatt-style formant synthesizer:

- **Source:** a band-limited buzz blended with a KLGLOTT88 glottal pulse, plus aspiration noise.
- **Vowels:** five cascaded resonators (F1 to F5), with a nasal pole/zero pair for M, N and NG.
- **Consonants:** frication noise through parallel resonators.
- **Transitions:** formant targets glide with per-phoneme timing, which is what makes consonants come across.

Text goes through a small hand-tuned dictionary, then the CMU Pronouncing Dictionary, then NRL-style letter-to-sound rules (Elovitz et al., 1976) for words neither knows, then a syllabifier. See [About the technologies](#about-the-technologies) for where each piece comes from.

The formant values for every phoneme are in `Source/engine/Phonemes.cpp`, which is handy if you want to recreate the sound with bandpass filters on a modular synth.

## About the technologies

Formant Daisy is a small rebuild of ideas from about 60 years of speech research. These are the pieces it stands on, roughly in the order they appeared.

### Peterson and Barney's vowel measurements (1952)

Gordon Peterson and Harold Barney at Bell Labs recorded 76 speakers saying ten English vowels and measured their formant frequencies ("Control methods used in a study of the vowels", JASA, 1952). Their averages became the standard reference for where each vowel sits, and the vowel targets in `Phonemes.cpp` start from them. The plugin's formant chart, with F1 down and F2 left, is the same kind of plot they made.

### Gunnar Fant and the source-filter model (1960)

Swedish engineer Gunnar Fant's *Acoustic Theory of Speech Production* (1960) described the voice as a sound source (the vocal cords) shaped by a filter (the throat, mouth and nose). Every formant synthesizer, including this one, is built on that idea: make a buzz, then filter it.

### Daisy Bell on the IBM 7094 (1961)

In 1961 at Bell Labs in Murray Hill, New Jersey, John L. Kelly Jr. and Carol Lochbaum programmed an IBM 7094 to sing "Daisy Bell", with accompaniment by computer music pioneer Max Mathews. It's the earliest known recording of a computer-synthesized voice singing a song. Arthur C. Clarke saw the demo, and that's why HAL 9000 sings "Daisy" as it shuts down in *2001: A Space Odyssey* (1968). It's also why this plugin is called Formant Daisy and loads the song by default.

### ARPAbet (1970s)

ARPAbet is a way of writing English sounds with plain ASCII letters (`iy`, `ey`, `sh`, `ng`), created for the ARPA speech research program in the 1970s. The plugin's phoneme names, the `[d ey | z iy]` bracket syntax and the CMU dictionary all use it.

### NRL letter-to-sound rules (1976)

Honey Sue Elovitz, Rodney Johnson, Astrid McHugh and John Shore at the US Naval Research Laboratory wrote a few hundred rules for turning English spelling into sounds ("Automatic translation of English text to phonetics by means of letter-to-sound rules", NRL Report 7948, 1976). Each rule looks at a letter group and the letters around it. Early text-to-speech systems used rule sets like this. Formant Daisy uses them for words that aren't in any dictionary, like new names and made-up words.

### Dennis H. Klatt and the Klatt synthesizer (1980)

[Dennis H. Klatt](https://en.wikipedia.org/wiki/Dennis_H._Klatt) (1938 to 1988) was a speech scientist at MIT and a pioneer of computer speech synthesis. His paper "Software for a cascade/parallel formant synthesizer" (JASA, 1980) described a synthesizer with formant resonators in series for vowels and in parallel for consonants. It became the standard design for formant synthesis, and Formant Daisy's engine follows it. Klatt's text-to-speech work (known as MITalk and KlattTalk) was developed into DECtalk. Later, with Laura Klatt, he improved the voice source. Their glottal pulse model, KLGLOTT88, is the "glottal" end of the plugin's VOICE knob. He received the Acoustical Society of America's Silver Medal in Speech Communication in 1987. He lost his own voice to cancer before his death.

### DECtalk (1984)

Digital Equipment Corporation turned Klatt's work into DECtalk, announced in December 1983 and shipped in early 1984. Its default voice, "Perfect Paul", is based on Klatt's own voice. Stephen Hawking's famous voice came from a synthesizer built on the same research, the CallText 5010, and he kept it for the rest of his life. DECtalk's phoneme notation, like `[dey ziy]`, is the second bracket style the plugin accepts.

### The CMU Pronouncing Dictionary

The [CMU Pronouncing Dictionary](https://github.com/cmusphinx/cmudict) (CMUdict) was made by the Speech Group at Carnegie Mellon University for speech recognition research. It lists over 134,000 English words with their North American pronunciations in ARPAbet, and marks which vowels are stressed. It's used in CMU Sphinx, the Festival speech synthesizer, the Natural Language Toolkit and several singing-voice synthesizers.

Formant Daisy includes about 125,000 of its words: the ones made only of letters and apostrophes, with the first pronunciation listed for each. `tools/make_cmudict.py` packs them into `Source/engine/CmuDict.cpp`. When packing, unstressed "uh" becomes the schwa `ax`, and an unstressed "er" before another vowel becomes `ax r` (so "inspiration" sings as "in-spuh-ray-shun"). When the plugin reads the dictionary, it merges sung syllables like "ow-er" into one note when the spelling has only one vowel ("our", "hour", "fire"). The dictionary is © 1993-2015 Carnegie Mellon University and is used under its BSD-style license ([third_party/cmudict/LICENSE](third_party/cmudict/LICENSE)).

### JUCE and VST3

The plugin is built with [JUCE](https://juce.com), a C++ framework for audio apps and plugins. It uses the VST3 plugin format created by Steinberg, so it loads in most DAWs. [pluginval](https://github.com/Tracktion/pluginval) by Tracktion checks every build.

## Building from source

Requires CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows, GCC or Clang on Linux). JUCE 8 is downloaded automatically.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target FormantDaisy_VST3 FormantDaisy_Standalone
```

Output goes to `build/FormantDaisy_artefacts/Release/`.

Linux needs these packages: `libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev`

Options:

- `-DJUCE_DIR=/path/to/JUCE`: use an existing JUCE checkout instead of downloading one.
- `-DFD_VST3_MANIFEST=OFF`: skip generating `moduleinfo.json` (needed when cross-compiling).
- `-DFD_BUILD_DEV_TOOLS=ON`: also build `ui_snapshot`, which drives the plugin without a host and saves editor screenshots. It made the images in this README.

The `render_daisy` tool (built by default, no JUCE needed) renders the chorus to a WAV without a DAW:

```
render_daisy out.wav [bpm] [voice 0..1] ["lyrics"]
```

To see how every word of a lyric will be sung (syllables and phonemes), build and run the pronunciation checker:

```
g++ -std=c++17 -O1 -ISource tools/pron_check.cpp Source/engine/Phonemes.cpp Source/engine/TextToPhonemes.cpp Source/engine/CmuDict.cpp -o pron_check
./pron_check tests/ode.txt
```

To update the dictionary, download `cmudict.dict` from [cmusphinx/cmudict](https://github.com/cmusphinx/cmudict) and run `python3 tools/make_cmudict.py cmudict.dict`.

### Source layout

```
Source/engine/       plain C++17 sound engine, no JUCE
  Phonemes.*         phoneme table (formants, bandwidths, noise shapes)
  TextToPhonemes.*   dictionaries, word definitions, letter-to-sound rules, syllables, rests, line breaks
  CmuDict.*          the packed CMU Pronouncing Dictionary (generated by tools/make_cmudict.py)
  FormantSinger.*    synthesis and note/tempo sequencing
Source/Plugin*.*     JUCE processor and pixel-art editor
Source/PixelFont.*   hand-drawn 5x7 bitmap font
tools/               test renderers, MIDI generator, screenshot tool, dictionary packer, pronunciation checker
tests/               lyric texts for pronunciation checks
third_party/         licenses for bundled data
docs/images/         README screenshots
```

## Releases

Versions follow `MAJOR.MINOR.PATCH`, and the minor number goes up with every change. The version lives in one place, `project(FormantDaisy VERSION ...)` in `CMakeLists.txt`, and is shown in the plugin's footer.

To publish a release:

1. Bump the version in `CMakeLists.txt` and add an entry to `CHANGELOG.md`.
2. Commit, then tag and push:

   ```
   git tag v0.12.0
   git push origin main v0.12.0
   ```

The **Release** workflow (`.github/workflows/release.yml`) then builds Windows and Linux and attaches these files to a GitHub Release:

- `FormantDaisy-<version>-Windows-x64-VST3.zip`
- `FormantDaisy-<version>-Windows-x64-Standalone.zip`
- `FormantDaisy-<version>-Linux-x64-VST3.zip`
- `FormantDaisy-<version>-Linux-x64-Standalone.zip`

The workflow fails if the tag doesn't match the version in `CMakeLists.txt`. You can also run it by hand from the Actions tab to get test builds without making a release.

## Credits

- Visual style inspired by Bit Wonder's short "Formant Synthesis Makes Buzzes Sing".
- D. H. Klatt, "Software for a cascade/parallel formant synthesizer", JASA 1980.
- D. H. Klatt and L. C. Klatt, "Analysis, synthesis, and perception of voice quality variations among female and male talkers", JASA 1990 (the KLGLOTT88 glottal pulse).
- G. E. Peterson and H. L. Barney, "Control methods used in a study of the vowels", JASA 1952.
- G. Fant, *Acoustic Theory of Speech Production*, Mouton, 1960.
- The [CMU Pronouncing Dictionary](https://github.com/cmusphinx/cmudict), Copyright (C) 1993-2015 Carnegie Mellon University, used under its BSD-style license ([third_party/cmudict/LICENSE](third_party/cmudict/LICENSE)). The license text is also included in every release zip.
- Elovitz, Johnson, McHugh and Shore, "Automatic translation of English text to phonetics by means of letter-to-sound rules", NRL Report 7948, 1976.
- "Daisy Bell" by Harry Dacre (1892) is in the public domain.
- Built with [JUCE](https://juce.com).
