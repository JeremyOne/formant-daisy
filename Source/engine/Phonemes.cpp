#include "Phonemes.h"
#include <vector>

namespace fd
{
static const FricShape kFric[FR_COUNT] = {
    /* NONE    */ { 1000, 500, 0.0f, 2000, 500, 0.0f, 0.0f },
    /* S       */ { 5500, 1200, 1.0f, 7500, 2000, 0.35f, 0.0f },
    /* SH      */ { 2600, 700, 1.0f, 4200, 1500, 0.5f, 0.0f },
    /* F       */ { 4500, 3000, 0.3f, 7000, 3000, 0.15f, 0.04f },
    /* TH      */ { 4500, 3000, 0.25f, 6500, 3000, 0.12f, 0.03f },
    /* BURST_P */ { 1000, 1500, 0.4f, 3000, 3000, 0.2f, 0.10f },
    /* BURST_T */ { 4500, 1500, 0.9f, 6500, 2500, 0.4f, 0.03f },
    /* BURST_K */ { 2000, 800, 0.9f, 3500, 1500, 0.4f, 0.05f },
};

// clang-format off
static const PhonemeDef kTable[] = {
//  name  label  kind                      F1   F2    F3    B1   B2   B3   AV    AF    AH    dur  fric        FNZ    offglide
  { "_",  "",    Kind::Silence,            400, 1400, 2400, 100, 100, 150, 0.0f, 0.0f, 0.0f, 0,   FR_NONE,    0,     nullptr },
  // monophthongs
  { "iy", "EE",  Kind::Vowel,              270, 2290, 3010, 60,  90,  150, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "ih", "IH",  Kind::Vowel,              390, 1990, 2550, 70,  100, 140, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "eh", "EH",  Kind::Vowel,              530, 1840, 2480, 60,  90,  200, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "ae", "AE",  Kind::Vowel,              660, 1720, 2410, 70,  150, 320, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "aa", "AH",  Kind::Vowel,              730, 1090, 2440, 130, 70,  160, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "ao", "AW",  Kind::Vowel,              570, 840,  2410, 90,  100, 80,  1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "ah", "UH",  Kind::Vowel,              640, 1190, 2390, 80,  50,  140, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "uh", "OO",  Kind::Vowel,              440, 1020, 2240, 80,  100, 80,  1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "uw", "OO",  Kind::Vowel,              300, 870,  2240, 65,  110, 140, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "er", "ER",  Kind::Vowel,              490, 1350, 1690, 70,  110, 80,  1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  { "ax", "UH",  Kind::Vowel,              500, 1400, 2300, 100, 100, 150, 1.0f, 0,    0,    0,   FR_NONE,    0,     nullptr },
  // diphthongs (start target here, end target in offglide)
  { "ey", "AY",  Kind::Diphthong,          480, 2000, 2600, 70,  100, 200, 1.0f, 0,    0,    0,   FR_NONE,    0,     "iy" },
  { "ay", "EYE", Kind::Diphthong,          700, 1220, 2600, 100, 80,  160, 1.0f, 0,    0,    0,   FR_NONE,    0,     "ih" },
  { "oy", "OY",  Kind::Diphthong,          550, 960,  2400, 90,  100, 120, 1.0f, 0,    0,    0,   FR_NONE,    0,     "iy" },
  { "aw", "OW",  Kind::Diphthong,          700, 1220, 2500, 100, 80,  160, 1.0f, 0,    0,    0,   FR_NONE,    0,     "uh" },
  { "ow", "OH",  Kind::Diphthong,          540, 1000, 2400, 90,  100, 100, 1.0f, 0,    0,    0,   FR_NONE,    0,     "uw" },
  // stops (formants are loci the vowel transitions come from)
  { "p",  "P",   Kind::StopVoiceless,      200, 800,  2200, 100, 150, 250, 0.0f, 0.8f, 0.45f,60,  FR_BURST_P, 0,     nullptr },
  { "b",  "B",   Kind::StopVoiced,         200, 800,  2200, 100, 150, 250, 0.0f, 0.3f, 0.0f, 50,  FR_BURST_P, 0,     nullptr },
  { "t",  "T",   Kind::StopVoiceless,      200, 1700, 2800, 100, 150, 250, 0.0f, 0.8f, 0.45f,55,  FR_BURST_T, 0,     nullptr },
  { "d",  "D",   Kind::StopVoiced,         200, 1700, 2800, 100, 150, 250, 0.0f, 0.3f, 0.0f, 45,  FR_BURST_T, 0,     nullptr },
  { "k",  "K",   Kind::StopVoiceless,      200, 1900, 2300, 100, 150, 250, 0.0f, 0.8f, 0.45f,60,  FR_BURST_K, 0,     nullptr },
  { "g",  "G",   Kind::StopVoiced,         200, 1900, 2300, 100, 150, 250, 0.0f, 0.3f, 0.0f, 50,  FR_BURST_K, 0,     nullptr },
  // fricatives
  { "f",  "F",   Kind::FricVoiceless,      340, 1100, 2080, 200, 120, 150, 0.0f, 0.45f,0.0f, 90,  FR_F,       0,     nullptr },
  { "v",  "V",   Kind::FricVoiced,         220, 1100, 2080, 60,  90,  120, 0.55f,0.15f,0.0f, 70,  FR_F,       0,     nullptr },
  { "th", "TH",  Kind::FricVoiceless,      320, 1290, 2540, 200, 90,  200, 0.0f, 0.35f,0.0f, 85,  FR_TH,      0,     nullptr },
  { "dh", "DH",  Kind::FricVoiced,         270, 1290, 2540, 60,  80,  170, 0.6f, 0.08f,0.0f, 50,  FR_TH,      0,     nullptr },
  { "s",  "S",   Kind::FricVoiceless,      320, 1390, 2530, 200, 80,  200, 0.0f, 1.0f, 0.0f, 115, FR_S,       0,     nullptr },
  { "z",  "Z",   Kind::FricVoiced,         240, 1390, 2530, 70,  60,  180, 0.5f, 0.3f, 0.0f, 85,  FR_S,       0,     nullptr },
  { "sh", "SH",  Kind::FricVoiceless,      300, 1840, 2750, 200, 100, 300, 0.0f, 1.0f, 0.0f, 115, FR_SH,      0,     nullptr },
  { "zh", "ZH",  Kind::FricVoiced,         300, 1840, 2750, 70,  100, 300, 0.5f, 0.3f, 0.0f, 80,  FR_SH,      0,     nullptr },
  // affricates (closure + fricative)
  { "ch", "CH",  Kind::AffricateVoiceless, 300, 1840, 2750, 200, 100, 300, 0.0f, 1.0f, 0.0f, 45,  FR_SH,      0,     nullptr },
  { "jh", "J",   Kind::AffricateVoiced,    260, 1800, 2820, 70,  100, 300, 0.45f,0.6f, 0.0f, 40,  FR_SH,      0,     nullptr },
  // aspirate (takes its formants from the vowel)
  { "hh", "H",   Kind::Aspirate,           500, 1400, 2300, 300, 300, 300, 0.0f, 0.0f, 0.55f,65,  FR_NONE,    0,     nullptr },
  // nasals
  { "m",  "M",   Kind::Nasal,              250, 1100, 2100, 60,  300, 400, 0.6f, 0.0f, 0.0f, 75,  FR_NONE,    1000,  nullptr },
  { "n",  "N",   Kind::Nasal,              250, 1500, 2600, 60,  300, 400, 0.6f, 0.0f, 0.0f, 70,  FR_NONE,    1700,  nullptr },
  { "ng", "NG",  Kind::Nasal,              250, 2000, 2700, 60,  300, 400, 0.6f, 0.0f, 0.0f, 75,  FR_NONE,    2600,  nullptr },
  // liquids and glides
  { "l",  "L",   Kind::Liquid,             330, 1050, 2880, 50,  100, 280, 0.75f,0.0f, 0.0f, 65,  FR_NONE,    0,     nullptr },
  { "r",  "R",   Kind::Liquid,             310, 1060, 1380, 70,  100, 120, 0.75f,0.0f, 0.0f, 65,  FR_NONE,    0,     nullptr },
  { "w",  "W",   Kind::Glide,              290, 610,  2150, 50,  80,  60,  0.7f, 0.0f, 0.0f, 60,  FR_NONE,    0,     nullptr },
  { "y",  "Y",   Kind::Glide,              260, 2070, 3020, 40,  250, 500, 0.7f, 0.0f, 0.0f, 55,  FR_NONE,    0,     nullptr },
};
// clang-format on

static constexpr int kCount = (int) (sizeof (kTable) / sizeof (kTable[0]));

int phonemeCount() { return kCount; }
const PhonemeDef& phoneme (int index) { return kTable[(index >= 0 && index < kCount) ? index : 0]; }
const FricShape& fricShape (int id) { return kFric[(id >= 0 && id < FR_COUNT) ? id : 0]; }
int silenceIndex() { return 0; }

int findPhoneme (const std::string& name)
{
    for (int i = 0; i < kCount; ++i)
        if (name == kTable[i].name)
            return i;
    return -1;
}

bool isVowelKind (Kind k) { return k == Kind::Vowel || k == Kind::Diphthong; }
} // namespace fd
