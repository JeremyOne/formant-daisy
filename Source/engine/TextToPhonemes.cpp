#include "TextToPhonemes.h"
#include "Phonemes.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <map>
#include <sstream>
#include <unordered_map>

namespace fd
{
//==============================================================================
// Dictionary. "spelled" uses '|' between syllables (for the lyric display);
// phonemes use '|' between syllables.
struct DictEntry
{
    const char* word;
    const char* spelled;
    const char* phones;
};

// clang-format off
static const DictEntry kDict[] = {
    // --- Daisy Bell (chorus + verses) ---
    { "daisy", "dai|sy", "d ey | z iy" },
    { "give", "give", "g ih v" },
    { "me", "me", "m iy" },
    { "your", "your", "y ao r" },
    { "answer", "an|swer", "ae n | s er" },
    { "do", "do", "d uw" },
    { "i'm", "i'm", "ay m" },
    { "half", "half", "hh ae f" },
    { "crazy", "cra|zy", "k r ey | z iy" },
    { "all", "all", "ao l" },
    { "for", "for", "f ao r" },
    { "the", "the", "dh ax" },
    { "love", "love", "l ah v" },
    { "of", "of", "ah v" },
    { "you", "you", "y uw" },
    { "it", "it", "ih t" },
    { "won't", "won't", "w ow n t" },
    { "be", "be", "b iy" },
    { "a", "a", "ax" },
    { "stylish", "sty|lish", "s t ay | l ih sh" },
    { "marriage", "mar|riage", "m ae | r ih jh" },
    { "i", "i", "ay" },
    { "can't", "can't", "k ae n t" },
    { "afford", "af|ford", "ax | f ao r d" },
    { "carriage", "car|riage", "k ae | r ih jh" },
    { "but", "but", "b ah t" },
    { "you'll", "you'll", "y uw l" },
    { "look", "look", "l uh k" },
    { "sweet", "sweet", "s w iy t" },
    { "upon", "up|on", "ax | p aa n" },
    { "seat", "seat", "s iy t" },
    { "bicycle", "bi|cy|cle", "b ay | s ih | k ax l" },
    { "built", "built", "b ih l t" },
    { "two", "two", "t uw" },
    { "there", "there", "dh eh r" },
    { "is", "is", "ih z" },
    { "flower", "flow|er", "f l aw | er" },
    { "within", "with|in", "w ih dh | ih n" },
    { "my", "my", "m ay" },
    { "heart", "heart", "hh aa r t" },
    { "planted", "plant|ed", "p l ae n | t ih d" },
    { "one", "one", "w ah n" },
    { "day", "day", "d ey" },
    { "by", "by", "b ay" },
    { "glancing", "glanc|ing", "g l ae n | s ih ng" },
    { "dart", "dart", "d aa r t" },
    { "bell", "bell", "b eh l" },
    { "whether", "wheth|er", "w eh | dh er" },
    { "she", "she", "sh iy" },
    { "loves", "loves", "l ah v z" },
    { "or", "or", "ao r" },
    { "not", "not", "n aa t" },
    { "sometimes", "some|times", "s ah m | t ay m z" },
    { "it's", "it's", "ih t s" },
    { "hard", "hard", "hh aa r d" },
    { "to", "to", "t uw" },
    { "tell", "tell", "t eh l" },
    { "yet", "yet", "y eh t" },
    { "am", "am", "ae m" },
    { "longing", "long|ing", "l ao ng | ih ng" },
    { "share", "share", "sh eh r" },
    { "lot", "lot", "l aa t" },
    { "beautiful", "beau|ti|ful", "b y uw | t ih | f uh l" },
    // --- common words the rules get wrong or that are worth pinning ---
    { "hello", "hel|lo", "hh eh | l ow" },
    { "world", "world", "w er l d" },
    { "are", "are", "aa r" },
    { "was", "was", "w ah z" },
    { "were", "were", "w er" },
    { "have", "have", "hh ae v" },
    { "what", "what", "w ah t" },
    { "where", "where", "w eh r" },
    { "who", "who", "hh uw" },
    { "why", "why", "w ay" },
    { "when", "when", "w eh n" },
    { "how", "how", "hh aw" },
    { "and", "and", "ae n d" },
    { "said", "said", "s eh d" },
    { "says", "says", "s eh z" },
    { "some", "some", "s ah m" },
    { "come", "come", "k ah m" },
    { "done", "done", "d ah n" },
    { "gone", "gone", "g ao n" },
    { "none", "none", "n ah n" },
    { "once", "once", "w ah n s" },
    { "only", "on|ly", "ow n | l iy" },
    { "any", "an|y", "eh | n iy" },
    { "many", "ma|ny", "m eh | n iy" },
    { "very", "ve|ry", "v eh | r iy" },
    { "every", "ev|ery", "eh v | r iy" },
    { "could", "could", "k uh d" },
    { "would", "would", "w uh d" },
    { "should", "should", "sh uh d" },
    { "through", "through", "th r uw" },
    { "though", "though", "dh ow" },
    { "thought", "thought", "th ao t" },
    { "eye", "eye", "ay" },
    { "eyes", "eyes", "ay z" },
    { "friend", "friend", "f r eh n d" },
    { "people", "peo|ple", "p iy | p ax l" },
    { "music", "mu|sic", "m y uw | z ih k" },
    { "computer", "com|pu|ter", "k ax m | p y uw | t er" },
    { "sing", "sing", "s ih ng" },
    { "song", "song", "s ao ng" },
    { "robot", "ro|bot", "r ow | b aa t" },
    { "human", "hu|man", "hh y uw | m ax n" },
    { "machine", "ma|chine", "m ax | sh iy n" },
    { "night", "night", "n ay t" },
    { "light", "light", "l ay t" },
    { "time", "time", "t ay m" },
    { "baby", "ba|by", "b ey | b iy" },
    { "oh", "oh", "ow" },
    { "ooh", "ooh", "uw" },
    { "ah", "ah", "aa" },
    { "la", "la", "l aa" },
    { "yeah", "yeah", "y eh" },
    { "dave", "dave", "d ey v" },
    { "hal", "hal", "hh ae l" },
    { "again", "a|gain", "ax | g eh n" },
    { "water", "wa|ter", "w ao | t er" },
    { "over", "o|ver", "ow | v er" },
    { "never", "nev|er", "n eh | v er" },
    { "ever", "ev|er", "eh | v er" },
    { "forever", "for|ev|er", "f er | eh | v er" },
    { "together", "to|geth|er", "t ax | g eh | dh er" },
    { "goodbye", "good|bye", "g uh d | b ay" },
    { "free", "free", "f r iy" },
    { "dream", "dream", "d r iy m" },
    { "dreams", "dreams", "d r iy m z" },
    { "star", "star", "s t aa r" },
    { "stars", "stars", "s t aa r z" },
    { "sky", "sky", "s k ay" },
    { "fly", "fly", "f l ay" },
    { "try", "try", "t r ay" },
    { "cry", "cry", "k r ay" },
    { "die", "die", "d ay" },
    { "lie", "lie", "l ay" },
    { "zero", "ze|ro", "z ih | r ow" },
    { "three", "three", "th r iy" },
    { "four", "four", "f ao r" },
    { "five", "five", "f ay v" },
    { "six", "six", "s ih k s" },
    { "seven", "sev|en", "s eh | v ax n" },
    { "eight", "eight", "ey t" },
    { "nine", "nine", "n ay n" },
    { "ten", "ten", "t eh n" },
};
// clang-format on

//==============================================================================
// Letter-to-sound rules (NRL style). left[match]right = phonemes
// Context symbols:
//   #  one or more vowels         *  one or more consonants
//   .  a voiced consonant         ^  exactly one consonant
//   +  a front vowel (E I Y)      :  zero or more consonants
//   &  a sibilant                 @  T S R D L Z N J TH CH SH
//   %  a suffix (ER E ES ED ING ELY)   ' ' word boundary
struct Rule
{
    const char* left;
    const char* match;
    const char* right;
    const char* out;
};

// clang-format off
static const Rule kRules[] = {
    // A
    {"", "A", " ", "ax"}, {" ", "ARE", " ", "aa r"}, {" ", "AR", "O", "ax r"}, {"", "AR", "#", "eh r"},
    {"^", "AS", "#", "ey s"}, {"", "A", "WA", "ax"}, {"", "AW", "", "ao"}, {" :", "ANY", "", "eh n iy"},
    {"", "A", "^+#", "ey"}, {"#:", "ALLY", "", "ax l iy"}, {" ", "AL", "#", "ax l"}, {"", "AGAIN", "", "ax g eh n"},
    {"#:", "AG", "E", "ih jh"}, {"", "A", "^+:#", "ae"}, {" :", "A", "^+ ", "ey"}, {"", "A", "^%", "ey"},
    {" ", "ARR", "", "ax r"}, {"", "ARR", "", "ae r"}, {" :", "AR", " ", "aa r"}, {"", "AR", " ", "er"},
    {"", "AR", "", "aa r"}, {"", "AIR", "", "eh r"}, {"", "AI", "", "ey"}, {"", "AY", "", "ey"},
    {"", "AU", "", "ao"}, {"#:", "AL", " ", "ax l"}, {"#:", "ALS", " ", "ax l z"}, {"", "ALK", "", "ao k"},
    {"", "AL", "^", "ao l"}, {" :", "ABLE", "", "ey b ax l"}, {"", "ABLE", "", "ax b ax l"}, {"", "ANG", "+", "ey n jh"},
    {"", "A", "", "ae"},
    // B
    {" ", "BE", "^#", "b ih"}, {"", "BEING", "", "b iy ih ng"}, {" ", "BOTH", " ", "b ow th"},
    {" ", "BUS", "#", "b ih z"}, {"", "BUIL", "", "b ih l"}, {"", "BB", "", "b"}, {"", "B", "", "b"},
    // C
    {" ", "CH", "^", "k"}, {"^E", "CH", "", "k"}, {"", "CH", "", "ch"}, {" S", "CI", "#", "s ay"},
    {"", "CI", "A", "sh"}, {"", "CI", "O", "sh"}, {"", "CI", "EN", "sh"}, {"", "C", "+", "s"},
    {"", "CK", "", "k"}, {"", "COM", "%", "k ah m"}, {"", "CC", "", "k"}, {"", "C", "", "k"},
    // D
    {"#:", "DED", " ", "d ih d"}, {".E", "D", " ", "d"}, {"#:^E", "D", " ", "t"}, {" ", "DE", "^#", "d ih"},
    {" ", "DO", " ", "d uw"}, {" ", "DOES", "", "d ah z"}, {" ", "DOING", "", "d uw ih ng"}, {" ", "DOW", "", "d aw"},
    {"", "DU", "A", "jh uw"}, {"", "DD", "", "d"}, {"", "D", "", "d"},
    // E
    {"#:", "E", " ", ""}, {"'^:", "E", " ", ""}, {" :", "E", " ", "iy"}, {"#", "ED", " ", "d"},
    {"#:", "E", "D ", ""}, {"", "EV", "ER", "eh v"}, {"", "E", "^%", "iy"}, {"", "ERI", "#", "iy r iy"},
    {"", "ERI", "", "eh r ih"}, {"#:", "ER", "#", "er"}, {"", "ER", "#", "eh r"}, {"", "ER", "", "er"},
    {" ", "EVEN", "", "iy v eh n"}, {"#:", "E", "W", ""}, {"@", "EW", "", "uw"}, {"", "EW", "", "y uw"},
    {"", "E", "O", "iy"}, {"#:&", "ES", " ", "ih z"}, {"#:", "E", "S ", ""}, {"#:", "ELY", " ", "l iy"},
    {"#:", "EMENT", "", "m eh n t"}, {"", "EFUL", "", "f uh l"}, {"", "EE", "", "iy"}, {"", "EARN", "", "er n"},
    {" ", "EAR", "^", "er"}, {"", "EAD", "", "eh d"}, {"#:", "EA", " ", "iy ax"}, {"", "EA", "SU", "eh"},
    {"", "EA", "", "iy"}, {"", "EIGH", "", "ey"}, {"", "EI", "", "iy"}, {" ", "EYE", "", "ay"},
    {"", "EY", "", "iy"}, {"", "EU", "", "y uw"}, {"", "E", "", "eh"},
    // F
    {"", "FUL", "", "f uh l"}, {"", "FF", "", "f"}, {"", "F", "", "f"},
    // G
    {"", "GIV", "", "g ih v"}, {" ", "G", "I^", "g"}, {"", "GE", "T", "g eh"}, {"SU", "GGES", "", "g jh eh s"},
    {"", "GG", "", "g"}, {" B#", "G", "", "g"}, {"", "G", "+", "jh"}, {"", "GREAT", "", "g r ey t"},
    {"#", "GH", "", ""}, {"", "G", "", "g"},
    // H
    {" ", "HAV", "", "hh ae v"}, {" ", "HERE", "", "hh iy r"}, {" ", "HOUR", "", "aw er"}, {"", "HOW", "", "hh aw"},
    {"", "H", "#", "hh"}, {"", "H", "", ""},
    // I
    {" ", "IN", "", "ih n"}, {" ", "I", " ", "ay"}, {"", "IN", "D", "ay n"}, {"", "IER", "", "iy er"},
    {"#:R", "IED", "", "iy d"}, {"", "IED", " ", "ay d"}, {"", "IEN", "", "iy eh n"}, {"", "IE", "T", "ay eh"},
    {" :", "I", "%", "ay"}, {"", "I", "%", "iy"}, {"", "IE", "", "iy"}, {"", "I", "^+:#", "ih"},
    {"", "IR", "#", "ay r"}, {"", "IZ", "%", "ay z"}, {"", "IS", "%", "ay z"}, {"", "I", "D%", "ay"},
    {"+^", "I", "^+", "ih"}, {"", "I", "T%", "ay"}, {"#:^", "I", "^+", "ih"}, {"", "I", "^+", "ay"},
    {"", "IR", "", "er"}, {"", "IGH", "", "ay"}, {"", "ILD", "", "ay l d"}, {"", "IGN", " ", "ay n"},
    {"", "IGN", "^", "ay n"}, {"", "IGN", "%", "ay n"}, {"", "IQUE", "", "iy k"}, {"", "I", "", "ih"},
    // J
    {"", "J", "", "jh"},
    // K
    {" ", "K", "N", ""}, {"", "K", "", "k"},
    // L
    {"", "LO", "C#", "l ow"}, {"L", "L", "", ""}, {"#:^", "L", "%", "ax l"}, {"", "LEAD", "", "l iy d"},
    {"", "L", "", "l"},
    // M
    {"", "MOV", "", "m uw v"}, {"", "MM", "", "m"}, {"", "M", "", "m"},
    // N
    {"E", "NG", "+", "n jh"}, {"", "NG", "R", "ng g"}, {"", "NG", "#", "ng g"}, {"", "NGL", "%", "ng g ax l"},
    {"", "NG", "", "ng"}, {"", "NK", "", "ng k"}, {" ", "NOW", " ", "n aw"}, {"", "NN", "", "n"}, {"", "N", "", "n"},
    // O
    {"", "OF", " ", "ax v"}, {"", "OROUGH", "", "er ow"}, {"#:", "OR", " ", "er"}, {"#:", "ORS", " ", "er z"},
    {"", "OR", "", "ao r"}, {" ", "ONE", "", "w ah n"}, {"", "OW", "", "ow"}, {" ", "OVER", "", "ow v er"},
    {"", "OV", "", "ah v"}, {"", "O", "^%", "ow"}, {"", "O", "^EN", "ow"}, {"", "O", "^I#", "ow"},
    {"", "OL", "D", "ow l"}, {"", "OUGHT", "", "ao t"}, {"", "OUGH", "", "ah f"}, {" ", "OU", "", "aw"},
    {"H", "OU", "S#", "aw"}, {"", "OUS", "", "ax s"}, {"", "OUR", "", "ao r"}, {"", "OULD", "", "uh d"},
    {"^", "OU", "^L", "ah"}, {"", "OUP", "", "uw p"}, {"", "OU", "", "aw"}, {"", "OY", "", "oy"},
    {"", "OING", "", "ow ih ng"}, {"", "OI", "", "oy"}, {"", "OOR", "", "ao r"}, {"", "OOK", "", "uh k"},
    {"", "OOD", "", "uh d"}, {"", "OO", "", "uw"}, {"", "O", "E", "ow"}, {"", "O", " ", "ow"},
    {"", "OA", "", "ow"}, {" ", "ONLY", "", "ow n l iy"}, {" ", "ONCE", "", "w ah n s"}, {"", "ON'T", "", "ow n t"},
    {"C", "O", "N", "aa"}, {"", "O", "NG", "ao"}, {" :^", "O", "N", "ah"}, {"I", "ON", "", "ax n"},
    {"#:", "ON", " ", "ax n"}, {"#^", "ON", "", "ax n"}, {"", "O", "ST ", "ow"}, {"", "OF", "^", "ao f"},
    {"", "OTHER", "", "ah dh er"}, {"", "OSS", " ", "ao s"}, {"#:^", "OM", "", "ah m"}, {"", "O", "", "aa"},
    // P
    {"", "PH", "", "f"}, {"", "PEOP", "", "p iy p"}, {"", "POW", "", "p aw"}, {"", "PUT", " ", "p uh t"},
    {"", "PP", "", "p"}, {"", "P", "", "p"},
    // Q
    {"", "QUAR", "", "k w ao r"}, {"", "QU", "", "k w"}, {"", "Q", "", "k"},
    // R
    {" ", "RE", "^#", "r iy"}, {"", "RR", "", "r"}, {"", "R", "", "r"},
    // S
    {"", "SH", "", "sh"}, {"#", "SION", "", "zh ax n"}, {"", "SOME", "", "s ah m"}, {"#", "SUR", "#", "zh er"},
    {"", "SUR", "#", "sh er"}, {"#", "SU", "#", "zh uw"}, {"#", "SSU", "#", "sh uw"}, {"#", "SED", " ", "z d"},
    {"#", "S", "#", "z"}, {"", "SAID", "", "s eh d"}, {"^", "SION", "", "sh ax n"}, {"", "S", "S", ""},
    {".", "S", " ", "z"}, {"#:.E", "S", " ", "z"}, {"#:^##", "S", " ", "z"}, {"#:^#", "S", " ", "s"},
    {"U", "S", " ", "s"}, {" :#", "S", " ", "z"}, {" ", "SCH", "", "s k"}, {"", "S", "C+", ""},
    {"#", "SM", "", "z m"}, {"#", "SN", "'", "z ax n"}, {"", "S", "", "s"},
    // T
    {" ", "THE", " ", "dh ax"}, {"", "TO", " ", "t uw"}, {"", "THAT", " ", "dh ae t"}, {" ", "THIS", " ", "dh ih s"},
    {" ", "THEY", "", "dh ey"}, {" ", "THERE", "", "dh eh r"}, {"", "THER", "", "dh er"}, {"", "THEIR", "", "dh eh r"},
    {" ", "THAN", " ", "dh ae n"}, {" ", "THEM", " ", "dh eh m"}, {"", "THESE", " ", "dh iy z"}, {" ", "THEN", "", "dh eh n"},
    {"", "THROUGH", "", "th r uw"}, {"", "THOSE", "", "dh ow z"}, {"", "THOUGH", " ", "dh ow"}, {" ", "THUS", "", "dh ah s"},
    {"", "TH", "", "th"}, {"#:", "TED", " ", "t ih d"}, {"S", "TI", "#N", "ch"}, {"", "TI", "O", "sh"},
    {"", "TI", "A", "sh"}, {"", "TIEN", "", "sh ax n"}, {"", "TUR", "#", "ch er"}, {"", "TU", "A", "ch uw"},
    {" ", "TWO", "", "t uw"}, {"", "TT", "", "t"}, {"", "T", "", "t"},
    // U
    {" ", "UN", "I", "y uw n"}, {" ", "UN", "", "ah n"}, {" ", "UPON", "", "ax p ao n"}, {"@", "UR", "#", "uh r"},
    {"", "UR", "#", "y uh r"}, {"", "UR", "*", "er"}, {"", "U", "^ ", "ah"}, {"", "U", "^^", "ah"},
    {"", "UY", "", "ay"}, {" G", "U", "#", ""}, {"G", "U", "%", ""}, {"G", "U", "#", "w"},
    {"#N", "U", "", "y uw"}, {"@", "U", "", "uw"}, {"", "U", "", "y uw"},
    // V
    {"", "VIEW", "", "v y uw"}, {"", "V", "", "v"},
    // W
    {" ", "WERE", "", "w er"}, {"", "WA", "S", "w aa"}, {"", "WA", "T", "w aa"}, {"", "WHERE", "", "w eh r"},
    {"", "WHAT", "", "w aa t"}, {"", "WHOL", "", "hh ow l"}, {"", "WHO", "", "hh uw"}, {"", "WH", "", "w"},
    {"", "WAR", "", "w ao r"}, {"", "WOR", "^", "w er"}, {"", "WR", "", "r"}, {"", "W", "", "w"},
    // X
    {"", "X", "", "k s"},
    // Y
    {"", "YOUNG", "", "y ah ng"}, {" ", "YOU", "", "y uw"}, {" ", "YES", "", "y eh s"}, {" ", "Y", "", "y"},
    {"#:^", "Y", " ", "iy"}, {"#:^", "Y", "I", "iy"}, {" :", "Y", " ", "ay"}, {" :", "Y", "#", "ay"},
    {" :", "Y", "^+:#", "ih"}, {" :", "Y", "^#", "ay"}, {"", "Y", "", "ih"},
    // Z
    {"", "ZZ", "", "z"}, {"", "Z", "", "z"},
};
// clang-format on

//==============================================================================
namespace
{
bool isVowelCh (char c) { return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'Y'; }
bool isLetter (char c) { return c >= 'A' && c <= 'Z'; }
bool isConsCh (char c) { return isLetter (c) && ! isVowelCh (c); }
bool isVoicedCons (char c) { return std::string ("BDVGJLMNRWZ").find (c) != std::string::npos && c != 0; }
bool isFrontVowel (char c) { return c == 'E' || c == 'I' || c == 'Y'; }

// --- right context, scanning forward from pos -----------------------------
bool matchRight (const char* pat, const std::string& w, int pos)
{
    const int n = (int) w.size();
    for (; *pat; ++pat)
    {
        const char p = *pat;
        if (pos > n)
            return false;
        const char c = pos < n ? w[pos] : ' ';
        switch (p)
        {
            case '#':
                if (! isVowelCh (c)) return false;
                while (pos < n && isVowelCh (w[pos])) ++pos;
                break;
            case '*':
                if (! isConsCh (c)) return false;
                while (pos < n && isConsCh (w[pos])) ++pos;
                break;
            case ':':
                while (pos < n && isConsCh (w[pos])) ++pos;
                break;
            case '^':
                if (! isConsCh (c)) return false;
                ++pos;
                break;
            case '.':
                if (! isVoicedCons (c)) return false;
                ++pos;
                break;
            case '+':
                if (! isFrontVowel (c)) return false;
                ++pos;
                break;
            case '&':
                if (w.compare (pos, 2, "CH") == 0 || w.compare (pos, 2, "SH") == 0) { pos += 2; break; }
                if (std::string ("SCGZXJ").find (c) == std::string::npos) return false;
                ++pos;
                break;
            case '@':
                if (w.compare (pos, 2, "TH") == 0 || w.compare (pos, 2, "CH") == 0 || w.compare (pos, 2, "SH") == 0) { pos += 2; break; }
                if (std::string ("TSRDLZNJ").find (c) == std::string::npos) return false;
                ++pos;
                break;
            case '%':
            {
                static const char* suffixes[] = { "ING", "ELY", "ER", "ES", "ED", "E" };
                bool ok = false;
                for (auto* s : suffixes)
                {
                    const int len = (int) std::char_traits<char>::length (s);
                    if (w.compare (pos, len, s) == 0)
                    {
                        pos += len;
                        ok = true;
                        break;
                    }
                }
                if (! ok) return false;
                break;
            }
            default:
                if (c != p) return false;
                ++pos;
                break;
        }
    }
    return true;
}

// --- left context, scanning backward from pos (pattern read right-to-left) --
bool matchLeft (const char* patStart, const std::string& w, int pos)
{
    const int len = (int) std::char_traits<char>::length (patStart);
    for (int i = len - 1; i >= 0; --i)
    {
        const char p = patStart[i];
        if (pos < -1)
            return false;
        const char c = pos >= 0 ? w[pos] : ' ';
        switch (p)
        {
            case '#':
                if (! isVowelCh (c)) return false;
                while (pos >= 0 && isVowelCh (w[pos])) --pos;
                break;
            case '*':
                if (! isConsCh (c)) return false;
                while (pos >= 0 && isConsCh (w[pos])) --pos;
                break;
            case ':':
                while (pos >= 0 && isConsCh (w[pos])) --pos;
                break;
            case '^':
                if (! isConsCh (c)) return false;
                --pos;
                break;
            case '.':
                if (! isVoicedCons (c)) return false;
                --pos;
                break;
            case '+':
                if (! isFrontVowel (c)) return false;
                --pos;
                break;
            case '&':
                if (pos >= 1 && (w.compare (pos - 1, 2, "CH") == 0 || w.compare (pos - 1, 2, "SH") == 0)) { pos -= 2; break; }
                if (std::string ("SCGZXJ").find (c) == std::string::npos) return false;
                --pos;
                break;
            case '@':
                if (pos >= 1 && (w.compare (pos - 1, 2, "TH") == 0 || w.compare (pos - 1, 2, "CH") == 0 || w.compare (pos - 1, 2, "SH") == 0)) { pos -= 2; break; }
                if (std::string ("TSRDLZNJ").find (c) == std::string::npos) return false;
                --pos;
                break;
            default:
                if (c != p) return false;
                --pos;
                break;
        }
    }
    return true;
}

std::vector<int> parsePhoneList (const std::string& s)
{
    std::vector<int> out;
    std::istringstream is (s);
    std::string tok;
    while (is >> tok)
    {
        const int idx = findPhoneme (tok);
        if (idx >= 0)
            out.push_back (idx);
    }
    return out;
}

struct Ph
{
    int id;
    int src; // letter index that produced it
};

// Apply rules to an uppercase letters-only word. Returns phonemes with source letter.
std::vector<Ph> applyRules (const std::string& upper)
{
    static std::map<char, std::vector<const Rule*>> byLetter = [] {
        std::map<char, std::vector<const Rule*>> m;
        for (auto& r : kRules)
            m[r.match[0]].push_back (&r);
        return m;
    }();

    // pad with a boundary on each side
    const std::string w = " " + upper + " ";
    std::vector<Ph> out;
    int i = 1;
    while (i < (int) w.size() - 1)
    {
        const char c = w[i];
        auto it = byLetter.find (c);
        bool matched = false;
        if (it != byLetter.end())
        {
            for (const Rule* r : it->second)
            {
                const int mlen = (int) std::char_traits<char>::length (r->match);
                if (w.compare (i, mlen, r->match) != 0)
                    continue;
                if (! matchLeft (r->left, w, i - 1))
                    continue;
                if (! matchRight (r->right, w, i + mlen))
                    continue;
                // spread the produced phonemes across the matched letters so
                // syllable labels can be cut at sensible letter positions
                auto ids = parsePhoneList (r->out);
                for (size_t k = 0; k < ids.size(); ++k)
                    out.push_back ({ ids[k], i - 1 + (int) ((k * (size_t) mlen) / ids.size()) });
                i += mlen;
                matched = true;
                break;
            }
        }
        if (! matched)
            ++i; // unknown char (e.g. apostrophe): skip silently
    }
    return out;
}

const DictEntry* lookup (const std::string& lowerWord)
{
    static std::unordered_map<std::string, const DictEntry*> map = [] {
        std::unordered_map<std::string, const DictEntry*> m;
        for (auto& e : kDict)
            m[e.word] = &e;
        return m;
    }();
    auto it = map.find (lowerWord);
    return it == map.end() ? nullptr : it->second;
}

int ph (const char* n) { return findPhoneme (n); }

// Can this consonant sequence begin an English syllable?
bool legalOnset (const std::vector<int>& c)
{
    if (c.empty())
        return true;
    auto name = [] (int id) { return std::string (phoneme (id).name); };
    if (c.size() == 1)
        return name (c[0]) != "ng";
    const std::string a = name (c[0]), b = name (c[1]);
    auto in = [] (const std::string& x, std::initializer_list<const char*> set) {
        for (auto* s : set) if (x == s) return true;
        return false;
    };
    if (c.size() == 2)
    {
        if (in (a, { "p", "b", "k", "g", "f" }) && in (b, { "l", "r" })) return true;
        if (in (a, { "t", "d", "th" }) && in (b, { "r", "w" })) return true;
        if (in (a, { "sh" }) && in (b, { "r" })) return true;
        if (in (a, { "k", "g", "s" }) && b == "w") return true;
        if (a == "s" && in (b, { "p", "t", "k", "m", "n", "l", "f" })) return true;
        if (in (a, { "p", "b", "k", "f", "v", "m", "hh" }) && b == "y") return true;
        return false;
    }
    if (c.size() == 3)
    {
        const std::string d = name (c[2]);
        return a == "s" && in (b, { "p", "t", "k" }) && in (d, { "l", "r", "w", "y" });
    }
    return false;
}

struct RawSyl
{
    std::vector<Ph> phs;
};

// Split a phoneme run into syllables around its vowels (maximal onset).
std::vector<RawSyl> autoSyllabify (std::vector<Ph> phs)
{
    std::vector<int> nuclei;
    for (int i = 0; i < (int) phs.size(); ++i)
        if (isVowel (phs[(size_t) i].id))
            nuclei.push_back (i);

    if (nuclei.empty())
    {
        // e.g. "hmm" or "st": give it a schwa so it can be sung
        int src = phs.empty() ? 0 : phs.back().src;
        phs.push_back ({ ph ("ax"), src });
        nuclei.push_back ((int) phs.size() - 1);
    }

    std::vector<int> starts; // index where each syllable begins
    starts.push_back (0);
    for (size_t k = 1; k < nuclei.size(); ++k)
    {
        const int a = nuclei[k - 1], b = nuclei[k];
        std::vector<int> cluster;
        for (int i = a + 1; i < b; ++i)
            cluster.push_back (phs[(size_t) i].id);
        int take = 0; // consonants that go to the next syllable's onset
        for (int t = (int) cluster.size(); t >= 1; --t)
        {
            std::vector<int> tail (cluster.end() - t, cluster.end());
            if (legalOnset (tail)) { take = t; break; }
        }
        // a single intervocalic consonant goes to the onset (open syllable), already handled
        starts.push_back (b - take);
    }

    std::vector<RawSyl> out;
    for (size_t k = 0; k < starts.size(); ++k)
    {
        const int s = starts[k];
        const int e = k + 1 < starts.size() ? starts[k + 1] : (int) phs.size();
        RawSyl r;
        r.phs.assign (phs.begin() + s, phs.begin() + e);
        out.push_back (r);
    }
    return out;
}

Syllable toSyllable (const std::vector<Ph>& phs)
{
    Syllable s;
    int i = 0;
    for (; i < (int) phs.size() && ! isVowel (phs[(size_t) i].id); ++i)
        s.onset.push_back (phs[(size_t) i].id);
    if (i < (int) phs.size())
        s.nucleus = phs[(size_t) i++].id;
    else
        s.nucleus = ph ("ax");
    for (; i < (int) phs.size(); ++i)
    {
        if (isVowel (phs[(size_t) i].id))
            continue; // shouldn't happen after syllabification
        s.coda.push_back (phs[(size_t) i].id);
    }
    return s;
}

std::string toLower (std::string s)
{
    for (auto& c : s) c = (char) std::tolower ((unsigned char) c);
    return s;
}
std::string toUpper (std::string s)
{
    for (auto& c : s) c = (char) std::toupper ((unsigned char) c);
    return s;
}

std::vector<std::string> splitOn (const std::string& s, char d)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == d) { out.push_back (cur); cur.clear(); }
        else cur += c;
    }
    out.push_back (cur);
    return out;
}

const char* digitWord (char c)
{
    static const char* w[] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine" };
    return w[c - '0'];
}

// Letter names, for spelling out acronyms. '|' splits syllables (only W has more than one).
const char* letterName (char c)
{
    static const char* names[26] = {
        "ey", "b iy", "s iy", "d iy", "iy", "eh f", "jh iy", "ey ch", "ay", "jh ey", "k ey", "eh l", "eh m",
        "eh n", "ow", "p iy", "k y uw", "aa r", "eh s", "t iy", "y uw", "v iy", "d ah | b ax l | y uw",
        "eh k s", "w ay", "z iy",
    };
    return (c >= 'A' && c <= 'Z') ? names[c - 'A'] : nullptr;
}

// An all-caps word of 2 to 5 letters (PHP, NASA, HTML) is read as an acronym.
// Single letters are left alone so "I" and "A" stay words.
bool isAcronym (const std::string& token)
{
    if (token.size() < 2 || token.size() > 5) return false;
    for (char c : token)
        if (c < 'A' || c > 'Z') return false;
    return true;
}

// Convert one word token (may contain hyphens as syllable hints) into syllables.
void wordToSyllables (const std::string& token, int wordIndex, std::vector<Syllable>& out, bool spellAcronyms = true)
{
    if (spellAcronyms && isAcronym (token))
    {
        for (char c : token)
        {
            auto parts = splitOn (letterName (c), '|');
            for (size_t k = 0; k < parts.size(); ++k)
            {
                std::vector<Ph> phs;
                for (int id : parsePhoneList (parts[k])) phs.push_back ({ id, 0 });
                Syllable syl = toSyllable (phs);
                syl.label = k == 0 ? std::string (1, c) : std::string();
                syl.word = wordIndex;
                out.push_back (syl);
            }
        }
        return;
    }

    const std::string lower = toLower (token);
    std::string joined;
    for (char c : lower) if (c != '-') joined += c;

    if (const DictEntry* e = lookup (joined); e != nullptr && lower.find ('-') == std::string::npos)
    {
        auto phParts = splitOn (e->phones, '|');
        auto spParts = splitOn (e->spelled, '|');
        for (size_t k = 0; k < phParts.size(); ++k)
        {
            std::vector<Ph> phs;
            for (int id : parsePhoneList (phParts[k]))
                phs.push_back ({ id, 0 });
            Syllable s = toSyllable (phs);
            s.label = (spParts.size() == phParts.size()) ? spParts[k] : (k == 0 ? joined : "");
            s.word = wordIndex;
            out.push_back (s);
        }
        return;
    }

    // letters only for the rules (apostrophes kept: some rules use them)
    std::string letters;
    std::vector<int> hintBreaks; // letter indexes where a hyphen hint starts a new part
    for (char c : lower)
    {
        if (c == '-') { hintBreaks.push_back ((int) letters.size()); continue; }
        if (std::isalpha ((unsigned char) c) || c == '\'') letters += c;
    }
    if (letters.empty())
        return;

    // A hyphenated word whose joined form is in the dictionary with the same
    // number of syllables: use the dictionary sounds with the user's labels.
    if (const DictEntry* e = lookup (joined); e != nullptr)
    {
        auto phParts = splitOn (e->phones, '|');
        auto userParts = splitOn (lower, '-');
        if (phParts.size() == userParts.size())
        {
            for (size_t k = 0; k < phParts.size(); ++k)
            {
                std::vector<Ph> p;
                for (int id : parsePhoneList (phParts[k])) p.push_back ({ id, 0 });
                Syllable s = toSyllable (p);
                s.label = userParts[k];
                s.word = wordIndex;
                out.push_back (s);
            }
            return;
        }
    }
    std::vector<Ph> phs = applyRules (toUpper (letters));
    if (phs.empty())
        return;

    // cut into hint parts by provenance
    std::vector<std::vector<Ph>> parts (1);
    std::vector<std::pair<int, int>> partLetters; // [start, end) letter range per part
    {
        std::vector<int> bounds = hintBreaks;
        size_t b = 0;
        for (auto& p : phs)
        {
            while (b < bounds.size() && p.src >= bounds[b])
            {
                parts.emplace_back();
                ++b;
            }
            parts.back().push_back (p);
        }
        // letter ranges per part (for labels)
        int start = 0;
        for (size_t k = 0; k <= hintBreaks.size(); ++k)
        {
            int end = k < hintBreaks.size() ? hintBreaks[k] : (int) letters.size();
            partLetters.push_back ({ start, end });
            start = end;
        }
        while (partLetters.size() > parts.size()) partLetters.pop_back();
    }

    // merge vowel-less parts into a neighbour
    for (size_t k = 0; k < parts.size();)
    {
        bool hasV = std::any_of (parts[k].begin(), parts[k].end(), [] (const Ph& p) { return isVowel (p.id); });
        if (hasV || parts.size() == 1) { ++k; continue; }
        if (k > 0)
        {
            parts[k - 1].insert (parts[k - 1].end(), parts[k].begin(), parts[k].end());
            partLetters[k - 1].second = partLetters[k].second;
        }
        else
        {
            parts[k + 1].insert (parts[k + 1].begin(), parts[k].begin(), parts[k].end());
            partLetters[k + 1].first = partLetters[k].first;
        }
        parts.erase (parts.begin() + (long) k);
        partLetters.erase (partLetters.begin() + (long) k);
    }

    for (size_t k = 0; k < parts.size(); ++k)
    {
        auto raws = autoSyllabify (parts[k]);
        const int pStart = partLetters[k].first, pEnd = partLetters[k].second;
        for (size_t r = 0; r < raws.size(); ++r)
        {
            Syllable s = toSyllable (raws[r].phs);
            int ls = r == 0 ? pStart : std::clamp (raws[r].phs.front().src, pStart, pEnd);
            int le = r + 1 < raws.size() ? std::clamp (raws[r + 1].phs.front().src, ls, pEnd) : pEnd;
            s.label = letters.substr ((size_t) ls, (size_t) std::max (0, le - ls));
            s.word = wordIndex;
            out.push_back (s);
        }
    }
}

// DECtalk-ish / ARPAbet phoneme group: "[d ey | z iy]" or "[dey ziy]"
void bracketToSyllables (const std::string& inside, int wordIndex, std::vector<Syllable>& out)
{
    static const std::map<std::string, std::string> alias = {
        { "rr", "er" }, { "ix", "ih" }, { "hx", "hh" }, { "nx", "ng" }, { "dx", "d" },
        { "el", "ax l" }, { "en", "ax n" }, { "yu", "y uw" }, { "ah", "ah" }, { "q", "" },
        { "lx", "l" }, { "rx", "r" }, { "tx", "t" }, { "dz", "d z" }, { "ts", "t s" },
        { "h", "hh" }, { "j", "jh" }, { "c", "k" }, { "x", "k s" },
    };

    std::vector<std::vector<Ph>> parts (1);
    const std::string s = toLower (inside);
    size_t i = 0;
    while (i < s.size())
    {
        const char c = s[i];
        if (c == '|' || c == '-' || c == '/')
        {
            parts.emplace_back();
            ++i;
            continue;
        }
        if (c == '<') // DECtalk tone/duration spec <dur,pitch>: ignored, MIDI drives pitch
        {
            while (i < s.size() && s[i] != '>') ++i;
            ++i;
            continue;
        }
        if (! std::isalpha ((unsigned char) c)) { ++i; continue; }

        bool got = false;
        for (int len = 2; len >= 1 && ! got; --len)
        {
            if (i + (size_t) len > s.size()) continue;
            std::string code = s.substr (i, (size_t) len);
            if (! std::all_of (code.begin(), code.end(), [] (char ch) { return std::isalpha ((unsigned char) ch); }))
                continue;
            std::string mapped = code;
            if (auto it = alias.find (code); it != alias.end()) mapped = it->second;
            auto ids = parsePhoneList (mapped);
            if (! ids.empty() || (alias.count (code) && mapped.empty()))
            {
                for (int id : ids) parts.back().push_back ({ id, 0 });
                i += (size_t) len;
                got = true;
            }
        }
        if (! got) ++i;
    }

    for (auto& part : parts)
    {
        if (part.empty()) continue;
        for (auto& raw : autoSyllabify (part))
        {
            Syllable syl = toSyllable (raw.phs);
            std::string label;
            for (auto& p : raw.phs) label += phoneme (p.id).name;
            syl.label = label;
            syl.word = wordIndex;
            out.push_back (syl);
        }
    }
}
} // namespace

//==============================================================================
int Lyrics::stepLength (bool tempoMode) const
{
    int n = 0;
    for (auto& s : syllables)
    {
        if (! s.rest) ++n;
        else if (s.noteRest) n += tempoMode ? std::max (1, s.restSteps) : 1;
    }
    return n;
}

int Lyrics::barLength (double stepsPerBar, bool tempoMode) const
{
    if (stepsPerBar <= 0.0) return 0;
    int bars = 0, steps = 0;
    auto closeLine = [&] {
        if (steps > 0) bars += (int) std::ceil ((double) steps / stepsPerBar - 1.0e-9);
        steps = 0;
    };
    for (auto& s : syllables)
    {
        if (! s.rest) ++steps;
        else if (s.noteRest) steps += tempoMode ? std::max (1, s.restSteps) : 1;
        if (s.lineBreak) closeLine();
    }
    closeLine();
    return bars;
}

int Lyrics::noteLength() const
{
    int n = 0;
    for (auto& s : syllables) if (! s.rest || s.noteRest) ++n;
    return n;
}

int Lyrics::singableCount() const
{
    int n = 0;
    for (auto& s : syllables) if (! s.rest) ++n;
    return n;
}

std::vector<int> wordToPhonemes (const std::string& word)
{
    std::vector<Syllable> syl;
    wordToSyllables (word, 0, syl);
    std::vector<int> out;
    for (auto& s : syl)
    {
        out.insert (out.end(), s.onset.begin(), s.onset.end());
        out.push_back (s.nucleus);
        out.insert (out.end(), s.coda.begin(), s.coda.end());
    }
    return out;
}

std::string phonemesToString (const std::vector<int>& phs)
{
    std::string s;
    for (int id : phs)
    {
        if (! s.empty()) s += ' ';
        s += phoneme (id).name;
    }
    return s;
}

Lyrics parseLyrics (const std::string& text, bool spellAcronyms)
{
    Lyrics L;
    // Every punctuation mark is its own rest (".... " = four rests). A line
    // break only lengthens a rest in Tempo mode and never takes a note.
    auto addRest = [&L] (int steps, bool punctuation) {
        if (! punctuation)
        {
            // a newline: mark "wait for the next bar" (merges into a rest right before it)
            if (L.syllables.empty()) return;
            if (L.syllables.back().rest)
            {
                L.syllables.back().lineBreak = true;
                return;
            }
        }
        Syllable r;
        r.rest = true;
        r.restSteps = punctuation ? steps : 0;
        r.noteRest = punctuation;
        r.lineBreak = ! punctuation;
        r.nucleus = silenceIndex();
        r.word = -1;
        L.syllables.push_back (r);
    };

    size_t i = 0;
    const size_t n = text.size();
    int lineNo = 0;
    size_t lined = 0; // syllables already tagged with their line
    auto tagLines = [&] {
        for (; lined < L.syllables.size(); ++lined)
            L.syllables[lined].line = lineNo;
    };
    while (i < n)
    {
        tagLines();
        const char c = text[i];
        if (c == '[')
        {
            size_t j = text.find (']', i + 1);
            if (j == std::string::npos) j = n;
            const std::string inside = text.substr (i + 1, j - i - 1);
            L.words.push_back ("[" + inside + "]");
            bracketToSyllables (inside, (int) L.words.size() - 1, L.syllables);
            i = j + 1;
            continue;
        }
        if (std::isalpha ((unsigned char) c) || std::isdigit ((unsigned char) c))
        {
            size_t j = i;
            std::string tok;
            while (j < n && (std::isalpha ((unsigned char) text[j]) || text[j] == '\'' || text[j] == '-'
                             || std::isdigit ((unsigned char) text[j])))
            {
                tok += text[j];
                ++j;
            }
            // strip trailing apostrophes/hyphens
            while (! tok.empty() && (tok.back() == '\'' || tok.back() == '-')) tok.pop_back();

            // digits become words, one per digit
            if (std::any_of (tok.begin(), tok.end(), [] (char ch) { return std::isdigit ((unsigned char) ch); }))
            {
                std::string letters;
                for (char ch : tok)
                {
                    if (std::isdigit ((unsigned char) ch))
                    {
                        if (! letters.empty()) { L.words.push_back (letters); wordToSyllables (letters, (int) L.words.size() - 1, L.syllables, spellAcronyms); letters.clear(); }
                        L.words.push_back (digitWord (ch));
                        wordToSyllables (digitWord (ch), (int) L.words.size() - 1, L.syllables);
                    }
                    else letters += ch;
                }
                if (! letters.empty()) { L.words.push_back (letters); wordToSyllables (letters, (int) L.words.size() - 1, L.syllables, spellAcronyms); }
            }
            else if (! tok.empty())
            {
                L.words.push_back (tok);
                wordToSyllables (tok, (int) L.words.size() - 1, L.syllables, spellAcronyms);
            }
            i = j;
            continue;
        }
        if (c == ',' || c == ';' || c == ':')
            addRest (1, true);
        else if (c == '.' || c == '!' || c == '?')
            addRest (2, true);
        else if (c == '\n')
        {
            addRest (2, false);
            tagLines();
            ++lineNo;
        }
        ++i;
    }
    tagLines();
    // drop a trailing line-break rest; keep trailing punctuation (it rests before looping)
    while (! L.syllables.empty() && L.syllables.back().rest && ! L.syllables.back().noteRest
           && ! L.syllables.back().lineBreak) // a trailing newline waits for the bar before looping
        L.syllables.pop_back();
    return L;
}
} // namespace fd
