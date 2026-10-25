// Phoneme inventory for the formant engine.
// Values are Klatt-style targets (adult male), loosely after Klatt (1980) and
// Peterson & Barney (1952). The Formant knob scales them for other voices.
#pragma once
#include <string>

namespace fd
{
enum class Kind
{
    Silence,
    Vowel,
    Diphthong,
    StopVoiceless,
    StopVoiced,
    FricVoiceless,
    FricVoiced,
    AffricateVoiceless,
    AffricateVoiced,
    Nasal,
    Liquid,
    Glide,
    Aspirate
};

// Shapes the frication noise (two parallel resonators + a flat bypass path).
struct FricShape
{
    float f1, b1, a1;
    float f2, b2, a2;
    float bypass;
};

enum FricId
{
    FR_NONE = 0,
    FR_S,
    FR_SH,
    FR_F,
    FR_TH,
    FR_BURST_P,
    FR_BURST_T,
    FR_BURST_K,
    FR_COUNT
};

struct PhonemeDef
{
    const char* name;  // ARPAbet-style lowercase code
    const char* label; // short label for the display (vowel chart, ticker)
    Kind kind;
    float f1, f2, f3; // formant targets (Hz). For stops these are the loci.
    float b1, b2, b3; // bandwidths (Hz)
    float av;         // voicing amplitude 0..1
    float af;         // frication amplitude 0..1
    float ah;         // aspiration amplitude 0..1
    float durMs;      // nominal duration (consonants; stops = closure)
    int fric;         // FricId
    float fnz;        // nasal antiresonance (Hz), 0 = none
    const char* offglide; // diphthong second target, nullptr if none
};

int phonemeCount();
const PhonemeDef& phoneme (int index);
int findPhoneme (const std::string& name); // -1 if not found
const FricShape& fricShape (int id);
bool isVowelKind (Kind k);
inline bool isVowel (int ph) { return ph >= 0 && isVowelKind (phoneme (ph).kind); }
int silenceIndex();
} // namespace fd
