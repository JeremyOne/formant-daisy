#!/usr/bin/env python3
"""Writes DaisyBell_chorus.mid: one note per syllable of the default lyrics.

Drop it on the Formant Daisy track in your DAW and press play. No dependencies.
Usage: python3 make_daisy_midi.py [bpm] [out.mid]

The melody is a simple transcription of the 1892 chorus (public domain) in
F major, 3/4. Phrases 1-2 are the familiar tune; phrases 3-4 are approximate.
"""
import struct
import sys

# (midi note, beats) - one entry per sung syllable
MELODY = [
    # Dai-sy, Dai-sy, give me your an-swer, do
    (72, 3), (69, 3), (65, 3), (60, 3), (62, 1), (64, 1), (65, 1), (62, 2), (65, 1), (60, 6),
    # I'm half cra-zy, all for the love of you
    (67, 3), (72, 3), (69, 3), (65, 3), (62, 1), (64, 1), (65, 1), (67, 2), (69, 1), (67, 6),
    # It won't be a sty-lish mar-riage
    (69, 1), (70, 1), (69, 1), (67, 1), (72, 2), (69, 1), (67, 2), (65, 3),
    # I can't af-ford a car-riage
    (67, 1), (69, 2), (65, 1), (62, 2), (65, 1), (62, 2), (60, 3),
    # But you'll look sweet up-on the seat of a bi-cy-cle built for two
    (60, 1), (65, 2), (69, 1), (67, 2), (60, 1), (65, 2), (69, 1), (67, 2),
    (69, 1), (70, 1), (72, 1), (69, 1), (65, 1), (67, 2), (60, 1), (65, 6),
]

PPQ = 480
GATE = 0.92  # leave a little gap so consonants at syllable ends get room


def vlq(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.insert(0, (n & 0x7F) | 0x80)
        n >>= 7
    return bytes(out)


def main():
    bpm = float(sys.argv[1]) if len(sys.argv) > 1 else 150.0
    out = sys.argv[2] if len(sys.argv) > 2 else "DaisyBell_chorus.mid"

    events = []  # (tick, order, bytes)
    tick = 0
    for note, beats in MELODY:
        length = int(beats * PPQ)
        events.append((tick, 1, bytes([0x90, note, 100])))
        events.append((tick + int(length * GATE), 0, bytes([0x80, note, 0])))
        tick += length
    events.sort(key=lambda e: (e[0], e[1]))

    tempo = int(60_000_000 / bpm)
    track = b"\x00\xff\x51\x03" + tempo.to_bytes(3, "big")
    track += b"\x00\xff\x58\x04\x03\x02\x18\x08"  # 3/4 time
    track += b"\x00\xff\x03" + vlq(len(b"Daisy Bell")) + b"Daisy Bell"
    last = 0
    for t, _, data in events:
        track += vlq(t - last) + data
        last = t
    track += b"\x00\xff\x2f\x00"

    with open(out, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ))
        f.write(b"MTrk" + struct.pack(">I", len(track)) + track)
    print(f"wrote {out}: {len(MELODY)} notes at {bpm:g} BPM")


if __name__ == "__main__":
    main()
