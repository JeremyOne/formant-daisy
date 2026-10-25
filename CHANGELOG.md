# Changelog

The minor version goes up with every change. Tag `vX.Y.Z` to publish a release.

## 0.13.0

- Six new color schemes: HAZY, NEUTRAL, HAPPY, TAM, BEES and SPORTS (ten in total).
- The compact view's COLOR button fits longer names.

## 0.12.0

- The chart's dot and trail are colored by F3 in both views (pink = low, warm = normal, blue = high).
- Consonant hiss is shown as static around the moving dot, sized by the noise spread and tinted by its pitch, with the consonant's name above it.
- Removed the dotted glide lines between phonemes.
- Spectrum: removed the moving F1/F2 labels and added F3 to the key.
- The F1/F2/F3 readout shows F3.

## 0.11.0

First tagged release.

- Klatt-style formant singing engine: buzz/glottal source, cascade formant resonators, nasal pole/zero, parallel frication.
- Lyrics text box with dictionary + letter-to-sound pronunciation, hyphen syllable hints and raw phoneme input.
- Notes mode (one syllable per MIDI note) and Tempo mode (one syllable per DIV step, host synced).
- Bar and beat tracking from the host. A new line waits for the next bar.
- Every punctuation mark is a one-note rest. The robot blinks on each rest.
- All-caps acronyms of 2 to 5 letters are spelled out (ABC toggle).
- LENGTH display with notes and bars.
- Formant chart with all vowels and consonants, phoneme line, spectrum, scope, bar/beat indicator.
- Robot mouth driven by the formants (F2 width, F1 height). 11 robots, switched by clicking.
- Four color schemes and a compact 9:16 view for YouTube Shorts.
- Optional audio input (vocoder style) with an AUTO / BUZZ / INPUT carrier setting.
- Version shown in the plugin footer.
