// Klatt-style formant singer.
//
//  voice source (buzz saw / KLGLOTT88 glottal pulse) + aspiration noise
//      -> nasal pole/zero -> cascade resonators F5..F1   (vowels, sonorants)
//  frication noise -> 2 parallel resonators + bypass     (s, sh, f, bursts)
//
// Lyrics are a list of syllables. Each MIDI note sings the next syllable at
// that pitch: onset consonants, then the vowel held until note-off, then the
// diphthong offglide and coda consonants. With "lead" > 0 the plugin reports
// that much latency and starts onset consonants early, so vowels land exactly
// on the beat (like a real singer anticipating a consonant).
#pragma once
#include "Phonemes.h"
#include "TextToPhonemes.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace fd
{
struct SingerParams
{
    float source = 0.5f;       // 0 = buzz (saw), 1 = glottal pulse
    float breath = 0.15f;      // extra aspiration on voiced sounds
    float formantShift = 1.0f; // scales all formants (voice size)
    float speed = 1.0f;        // consonant duration scale (higher = longer)
    float glideMs = 60.0f;     // portamento between notes
    float vibDepthCents = 25.0f;
    float vibRateHz = 5.5f;
    float lofi = 0.0f;         // 0..1 sample-rate / bit reduction
    float gain = 0.5f;         // linear output gain
    bool loop = true;
};

class FormantSinger
{
public:
    FormantSinger();

    void prepare (double sampleRate);
    void reset();

    // Audio thread only. Replacing the lyrics clears pending events and restarts.
    void setLyrics (std::shared_ptr<const Lyrics> lyrics);
    void setParams (const SingerParams& p) { params = p; }
    void setLeadSamples (int samples) { leadSamples = samples < 0 ? 0 : samples; }

    // Events. offset = sample offset within the next process() call.
    static constexpr int64_t noBar = INT64_MIN;
    // bar: index of the bar the event falls in (noBar if unknown). Used so a line
    // break in the lyrics waits for the next bar.
    void noteOn (int note, float velocity, int offset, int64_t bar = noBar);
    void noteOff (int note, int offset);
    void triggerStep (int note, float velocity, int offset, int gateSamples, int64_t bar = noBar); // tempo mode
    void releaseAll (int offset);
    void setPitchBend (float semitones) { bendSemis = semitones; }
    void restart();
    bool anyNoteHeld() const { return heldCount > 0; }
    int topHeldNote() const { return heldCount > 0 ? held[(size_t) heldCount - 1] : -1; }

    // carrier: optional external excitation (vocoder-style). When given and
    // setCarrierMix() > 0, it replaces the internal buzz as the voiced source;
    // consonant noise stays internal. Pass nullptr for the internal buzz only.
    void process (float* out, int numSamples, const float* carrier = nullptr);
    void setCarrierMix (float target) { carrierMixTarget = target < 0 ? 0.0f : (target > 1 ? 1.0f : target); }

    // Visualisation (read from the UI thread)
    std::atomic<float> vizF1 { 500 }, vizF2 { 1500 }, vizF3 { 2500 };
    std::atomic<float> vizVoicing { 0 }, vizNoise { 0 }, vizLevel { 0 };
    // consonant noise for the display: frication level, aspiration level,
    // centre frequency of the frication (Hz) and its spread (octaves)
    std::atomic<float> vizFric { 0 }, vizAsp { 0 }, vizFricHz { 4000 }, vizFricSpread { 1 };
    std::atomic<int> vizSyllable { -1 }, vizPhoneme { 0 };
    std::atomic<int> vizRestCount { 0 };
    std::atomic<bool> vizWaiting { false }; // paused at a line break until the next bar // increments every time a rest is consumed

private:
    struct Frame
    {
        float F[5] {}, B[5] {};
        float av = 0, ah = 0, af = 0;
        float fr1f = 1000, fr1b = 500, fr1a = 0, fr2f = 2000, fr2b = 500, fr2a = 0, byp = 0;
        float fnz = 270;
    };
    struct Segment
    {
        Frame tgt;
        int64_t dur = -1; // samples, -1 = hold until something follows
        float fTauMs = 30, aTauMs = 15;
        int ph = 0;
        bool nucleus = false;
    };
    enum class EvType { Start, Release, PitchTo };
    struct Event
    {
        int64_t time = 0;
        EvType type = EvType::Start;
        int syllable = -1;
        int note = 60;
        float vel = 1.0f;
    };

    struct Resonator
    {
        double a = 1, b = 0, c = 0, y1 = 0, y2 = 0;
        void set (double f, double bw, double sr);
        inline double process (double x)
        {
            const double y = a * x + b * y1 + c * y2;
            y2 = y1;
            y1 = y;
            return y;
        }
        void clear() { y1 = y2 = 0; }
    };
    struct AntiResonator
    {
        double a = 1, b = 0, c = 0, x1 = 0, x2 = 0;
        void set (double f, double bw, double sr);
        inline double process (double x)
        {
            const double y = a * x + b * x1 + c * x2;
            x2 = x1;
            x1 = x;
            return y;
        }
        void clear() { x1 = x2 = 0; }
    };

    // --- sequencing ---------------------------------------------------------
    int pickNextSyllable (bool tempoMode, int64_t bar); // -1 = nothing to sing / tempo rest, -2 = notes-mode rest
    int64_t estimateOnset (int syl) const;
    int64_t estimateCoda (int syl) const;
    void schedule (const Event& e);
    void fireEvent (const Event& e);
    void startSyllable (int syl, int note, float vel);
    void releaseCurrent();

    // --- segment building -----------------------------------------------------
    Frame frameFor (int ph) const;
    void pushSeg (const Segment& s);
    void pushConsonant (int ph, int nucleusPh, bool onset, bool lastBeforeVowel);
    void pushSilence();
    int64_t ms (float m) const { return (int64_t) (m * 0.001 * sampleRate); }
    int64_t consonantDurEstimate (int ph, bool onset) const;

    void advanceSegments (int n);
    void updateControl();
    void heldPush (int note);
    void heldRemove (int note);

    SingerParams params;
    double sampleRate = 48000.0;
    int leadSamples = 0;
    int64_t clock = 0;

    std::shared_ptr<const Lyrics> lyrics;
    int nextSyl = 0;
    int restCounter = 0;
    int64_t waitUntilBar = noBar, lastNoteBar = noBar;
    int currentSyl = -1;
    int lastScheduledSyl = -1;
    int currentNucleus = -1;

    std::array<Event, 256> events {};
    int eventCount = 0;

    std::array<Segment, 96> queue {};
    int qHead = 0, qCount = 0;
    Segment seg;
    int64_t segElapsed = 0;

    std::array<int, 16> held {};
    int heldCount = 0;
    int soundingNote = -1;

    Frame cur;
    float velGain = 1.0f, velTarget = 1.0f;
    double f0 = 110.0, f0Target = 110.0;
    float bendSemis = 0.0f;
    double phase = 0.0;
    double vibPhase = 0.0;
    int64_t nucleusStartClock = 0;
    float jitter = 0.0f;
    uint32_t rng = 0x12345678u;

    Resonator casc[5], nasalPole, fr1, fr2;
    AntiResonator nasalZero;
    double tiltState = 0.0, tiltCoef = 0.0;
    float carrierMix = 0.0f, carrierMixTarget = 0.0f;
    double carrierEnv = 0.0; // level follower for automatic input gain
    double fricLp = 0.0, fricLpCoef = 0.0;
    int controlCounter = 0;

    // lofi
    double lofiPhase = 0.0, lofiHold = 0.0;

    // viz accumulation
    float levelAcc = 0.0f;
};
} // namespace fd
