# Formant Daisy on the M-VAVE FM-1 (experimental)

FM-1 port 0.1.0. This branch puts Formant Daisy's singing robot on the M-VAVE FM-1 pocket synth, as a new engine
called **DAISY** inside the [Felucca](https://github.com/hugelton/Felucca) custom firmware.

**Status:** the engine runs inside Felucca's real firmware code on a computer (Felucca's emulator harness) and
passes its tests there. It has **not** been built with the JieLi toolchain or run on an FM-1 yet. Treat the first
flash as a test, and keep the stock firmware and the rescue steps at hand.

## How it plays

Each key sings the next syllable of a lyric at that key's pitch. Holding a key holds the vowel; letting go sings the
end of the word. It is the plugin's synth version, minus the computer: the lyric is turned into phonemes on a
computer beforehand, so the FM-1 never needs the dictionary.

* **Mono:** one singer per part. Felucca's glide, pitch bend, tuning and LFO still work, as DAISY takes its pitch
  from Felucca.
* **PLAY** on the sequencer takes the lyric back to its first syllable.
* **LOOP** on: after the last syllable it starts again.

| Page | Knob | What it does |
| --- | --- | --- |
| SING | VOICE | buzz (0) to soft (127), the glottal source |
| SING | BRTH | breath noise mixed into the voice |
| SING | FMNT | formant shift: big robot (0) to tiny robot (127), 64 is normal |
| SING | CONS | consonant speed, 64 is normal |
| WORD | VIB | vibrato depth |
| WORD | VRAT | vibrato rate, about 2 to 9 Hz |
| WORD | LYRC | DAISY, ROBOT or USER (the lyric you sent) |
| WORD | LOOP | start the lyric again at its end |

The HOME knobs are VOICE, FMNT, CONS and LYRC. Presets: DAISY BELL, 1961 MAINFRAME, ROBOT HOUSE, TINY ROBOT,
BIG ROBOT.

Built-in lyrics: **DAISY** (the chorus of "Daisy Bell", Harry Dacre, 1892, public domain) and **ROBOT** (the
original robot-house verse and chorus). The text for both is in `lyrics/`.

## Sending your own lyric

Build the encoder (from the repository root, Linux, macOS or WSL):

```sh
g++ -std=c++17 -O2 -ISource ports/fm1/tools/fm1_encode.cpp Source/engine/*.cpp -o fm1_encode
./fm1_encode --show @my_lyric.txt          # check the syllables
./fm1_encode --syx my_lyric.syx @my_lyric.txt
```

The text is written exactly as in the plugin: the CMU dictionary, your own `word = phonemes` definitions, `_` rests
and so on. Bar markers (`-bar.12-`) are ignored here; the FM-1 just sings syllables in order.

Send `my_lyric.syx` to the FM-1 over USB MIDI with any SysEx tool (SysEx Librarian on macOS, MIDI-OX on Windows,
`amidi -s my_lyric.syx` on Linux). Every part playing DAISY switches to **USER** and sings it from its first
syllable. A lyric can be up to 4096 bytes (several verses). It is kept until the FM-1 is switched off.

### The SysEx messages

`7D` is the non-commercial manufacturer ID, `44 46` is "DF". The lyric goes in pieces because Felucca reads SysEx in
frames of up to 640 bytes:

```
F0 7D 44 46 01 F7               begin
F0 7D 44 46 02 <bytes> F7       up to 512 lyric bytes a message, as many as needed
F0 7D 44 46 03 F7               end: the FM-1 checks the lyric, then uses it
```

A lyric that is cut short or broken is refused at the end and the old one stays.

### The lyric format

Every byte is 0..127. Per syllable:

```
flags  n_onset  onset[n_onset]  nucleus  n_coda  coda[n_coda]
```

* `flags`: bit 0 = a rest (a note that sings nothing), bit 1 = a line break follows.
* Phoneme codes 1..40: iy ih eh ae aa ao ah uh uw er ax ey ay oy aw ow p b t d k g f v th dh s z sh zh ch jh hh m
  n ng l r w y. 0 = none.
* Up to 6 consonants each side, up to 512 syllables.

## What is in here

| Path | What |
| --- | --- |
| `src/daisy_fx.c`, `daisy_fx.h` | the plugin's FormantSinger in integer C: no floats, no divides in the audio path |
| `src/daisy_fx_tables.h` | phoneme and filter tables, made from the plugin's `Phonemes.cpp` by `tools/gen_tables.py` |
| `src/daisy_lyrics.h` | the two built-in lyrics, made by `fm1_encode --c` |
| `felucca/eng_daisy.c` | the Felucca engine: knobs, presets, the SysEx upload |
| `felucca/felucca-daisy.patch` | the small changes to Felucca's own files (engine list, MIDI CC, CPU cost, SysEx hook) |
| `felucca/apply.sh` | applies the patch and copies the files into a Felucca checkout |
| `tools/fm1_encode.cpp` | lyric text to engine bytes, C arrays or SysEx |
| `tests/compare_plugin.cpp` | plays Daisy Bell through the plugin and the port and compares them |
| `tests/felucca_daisy_test.c` | DAISY inside Felucca's emulator: notes, syllables, level, SysEx, PLAY |
| `tests/level_cmp.c`, `tests/cost.c` | DAISY's loudness and CPU time against Felucca's other engines |

### How the port works

The fixed-point engine follows the plugin step for step: a Klatt-style cascade of formant resonators (signals in
Q12, coefficients in Q30), a nasal antiresonator, a glottal pulse source with breath noise, and the same consonant
timing, updated every 16 samples. Frequencies are kept as Hz × 16 and amplitudes in Q15. Square roots, cosines and
bandwidth decays come from tables, so the audio path only multiplies, adds and shifts.

Memory: each part's singer lives in the part's engine memory (Felucca's `ENG_MEM`, about 12.9 KB a part); the
uploaded lyric and its staging copy take 8 KB of the FM-1's POOL RAM.

## Results on a computer

`tests/compare_plugin.cpp` (plugin vs port, Daisy Bell):

```
F1 difference: median about 6 Hz      F2 difference: median about 3 Hz
loudness envelope correlation: 0.987  level, FM-1 vs plugin: +0.0 dB   peak 0.85
```

`tests/felucca_daisy_test.c`: all 9 checks pass. With DAISY compiled in but not used, Felucca's heavy test song
renders byte-identical to unpatched Felucca.

CPU: one DAISY singer costs about 2.3 times an ANALOG voice in host timing, so its cost in Felucca's voice budget
is set to 370 (ANALOG is 163). This is the number to check on the hardware: if the FM-1 stutters with DAISY plus a
busy song, raise `ENG_VCOST` for DAISY in `fx.c`.

## Building the firmware

The JieLi toolchain and SDK only run on Linux x86-64 (or Docker on another system). Follow Felucca's `BUILDING.md`;
the only extra step is applying this port:

```sh
git clone --recursive https://github.com/hugelton/Felucca.git
ports/fm1/felucca/apply.sh /path/to/Felucca
cd /path/to/Felucca
# then Felucca's normal build (tools/build.py), which produces the .fwsc firmware file
```

The patch is made against Felucca 406b328 (engines d026007, bsp 5942e68). If Felucca has moved on and the patch
does not apply, the changes are small: see `felucca-daisy.patch`. DAISY is engine number 15; when Felucca is built
without ADPCM, slot 14 is reserved so DAISY keeps its number and saved songs keep working.

To try DAISY without hardware, in a Felucca checkout with the port applied (after `tools/build.py` has generated
`build/gen`):

```sh
cd tests/daisy
cc -O2 -ffp-contract=off -w -I../../build/gen -I../../firmware/src -I../../firmware/hal \
   -I../../firmware/bsp/chip -I../../firmware/bsp/boards/fm1 -o daisy_test felucca_daisy_test.c -lm
./daisy_test daisy.wav
```

## Installing

Install the resulting `.fwsc` the way Felucca is installed (see Felucca's README and the community install guides).
Before you do:

* Learn the FM-1's rescue mode first, so a bad flash can be undone.
* Keep a copy of the stock firmware.
* Check which FM-1 hardware revision you have against Felucca's notes; follow Felucca's guidance for newer units.
* This is unofficial firmware on your own device, at your own risk.

## Licence

Felucca is GPL-3.0, so this engine, as part of Felucca, is GPL-3.0 too (`eng_daisy.c` carries the SPDX line).
`daisy_fx.c` is ported from Formant Daisy's own engine; built into Felucca it is distributed under GPL-3.0 with the
rest of the firmware. The CMU Pronouncing Dictionary is only used on the computer, by `fm1_encode`, never on the device.
