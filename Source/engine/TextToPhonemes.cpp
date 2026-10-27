#include "TextToPhonemes.h"
#include "Phonemes.h"
#include "CmuDict.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <map>
#include <optional>
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
    // music words the big dictionary reads another way
    { "bass", "bass", "b ey s" },
    { "basses", "bass|es", "b ey | s ih z" },
    // poetic contractions
    { "o'er", "o'er", "ow r" },
    { "e'er", "e'er", "eh r" },
    { "ne'er", "ne'er", "n eh r" },
    { "e'en", "e'en", "iy n" },
    { "'tis", "'tis", "t ih z" },
    { "'twas", "'twas", "t w ah z" },
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

//==============================================================================
// Big dictionary: the CMU Pronouncing Dictionary (see CmuDict.h), unpacked into
// a hash map the first time a word is looked up.
struct CmuIndex
{
    std::unordered_map<std::string, std::pair<unsigned int, unsigned char>> words; // offset, count
    std::vector<int> codeToId;
};

const CmuIndex& cmuIndex()
{
    static const CmuIndex idx = [] {
        CmuIndex x;
        for (int i = 0; i < kCmuPhoneCount; ++i)
            x.codeToId.push_back (findPhoneme (kCmuPhones[i]));
        x.words.reserve (kCmuWordCount);
        std::string word;
        unsigned int p = 0;
        while (p + 2 <= kCmuDataSize)
        {
            const unsigned shared = kCmuData[p++], len = kCmuData[p++];
            word.resize (std::min<size_t> (shared, word.size()));
            word.append (reinterpret_cast<const char*> (kCmuData + p), len);
            p += len;
            const unsigned char count = kCmuData[p++];
            x.words.emplace (word, std::make_pair (p, count));
            p += count;
        }
        return x;
    }();
    return idx;
}

bool cmuLookup (const std::string& lowerWord, std::vector<int>& out)
{
    const auto& idx = cmuIndex();
    auto it = idx.words.find (lowerWord);
    if (it == idx.words.end()) return false;
    out.clear();
    for (unsigned k = 0; k < it->second.second; ++k)
    {
        const unsigned code = kCmuData[it->second.first + k];
        if (code < idx.codeToId.size() && idx.codeToId[code] >= 0)
            out.push_back (idx.codeToId[code]);
    }
    return ! out.empty();
}

bool voicelessEnd (const std::vector<int>& p)
{
    if (p.empty()) return false;
    const std::string n = phoneme (p.back()).name;
    return n == "p" || n == "t" || n == "k" || n == "f" || n == "th";
}
bool sibilantEnd (const std::vector<int>& p)
{
    if (p.empty()) return false;
    const std::string n = phoneme (p.back()).name;
    return n == "s" || n == "z" || n == "sh" || n == "zh" || n == "ch" || n == "jh";
}
void append (std::vector<int>& p, const char* phones)
{
    for (int id : parsePhoneList (phones)) p.push_back (id);
}

// Whole-word lookup: hand dictionary, then CMU, then (for words neither knows)
// a known word plus a common ending ("slumbers", "deathless"), then two known
// words joined together ("webmail", "ransomware").
bool lookupWord (const std::string& w, std::vector<int>& out, int depth = 0)
{
    if (const DictEntry* e = lookup (w))
    {
        std::string flat = e->phones;
        std::replace (flat.begin(), flat.end(), '|', ' ');
        out = parsePhoneList (flat);
        return ! out.empty();
    }
    if (cmuLookup (w, out))
        return true;
    if (depth >= 2 || w.size() < 4)
        return false;

    auto ends = [&w] (const char* s) {
        const size_t n = std::strlen (s);
        return w.size() > n + 2 && w.compare (w.size() - n, n, s) == 0;
    };
    auto stem = [&w] (size_t cut) { return w.substr (0, w.size() - cut); };
    // try the stem as is, with a silent e put back, and with a doubled consonant removed
    auto stemVariants = [] (const std::string& s) {
        std::vector<std::string> v { s, s + "e" };
        if (s.size() > 2 && s.back() == s[s.size() - 2]) v.push_back (s.substr (0, s.size() - 1));
        if (! s.empty() && s.back() == 'i') v.push_back (s.substr (0, s.size() - 1) + "y");
        return v;
    };
    std::vector<int> base;

    if (ends ("'s") && lookupWord (stem (2), base, depth + 1))
    {
        out = base;
        append (out, sibilantEnd (base) ? "ih z" : voicelessEnd (base) ? "s" : "z");
        return true;
    }
    if (ends ("ing"))
        for (auto& s : stemVariants (stem (3)))
            if (lookupWord (s, base, depth + 1)) { out = base; append (out, "ih ng"); return true; }
    if (ends ("ed"))
        for (auto& s : stemVariants (stem (2)))
            if (lookupWord (s, base, depth + 1))
            {
                out = base;
                const std::string last = phoneme (base.back()).name;
                append (out, (last == "t" || last == "d") ? "ih d" : voicelessEnd (base) || sibilantEnd (base) ? "t" : "d");
                return true;
            }
    static const std::pair<const char*, const char*> suffixes[] = {
        { "less", "l ax s" }, { "ness", "n ax s" }, { "ful", "f ax l" }, { "ly", "l iy" },
        { "ers", "er z" }, { "er", "er" }, { "ment", "m ax n t" }, { "able", "ax b ax l" },
    };
    for (auto& [suf, phones] : suffixes)
        if (ends (suf))
            for (auto& s : stemVariants (stem (std::strlen (suf))))
                if (lookupWord (s, base, depth + 1)) { out = base; append (out, phones); return true; }
    if (ends ("es") && lookupWord (stem (2), base, depth + 1) && sibilantEnd (base))
    {
        out = base;
        append (out, "ih z");
        return true;
    }
    if (ends ("s") && w[w.size() - 2] != 's' && lookupWord (stem (1), base, depth + 1))
    {
        out = base;
        append (out, sibilantEnd (base) ? "ih z" : voicelessEnd (base) ? "s" : "z");
        return true;
    }

    // two known words joined: longest first part wins, both parts 3+ letters
    if (depth == 0 && w.size() >= 6)
        for (size_t cut = w.size() - 3; cut >= 3; --cut)
        {
            std::vector<int> a, b;
            if (lookupWord (w.substr (0, cut), a, 2) && lookupWord (w.substr (cut), b, 1))
            {
                out = a;
                out.insert (out.end(), b.begin(), b.end());
                return true;
            }
        }
    return false;
}

// Split a word's letters into n display chunks, one per sung syllable. Uses
// vowel groups when they match the syllable count, otherwise even slices.
std::vector<std::pair<size_t, size_t>> vowelGroups (const std::string& word)
{
    auto isV = [&word] (size_t i) {
        return i < word.size() && std::strchr ("aeiou", std::tolower ((unsigned char) word[i])) != nullptr && word[i] != 0;
    };
    auto vowelAt = [&] (size_t i) {
        const char c = (char) std::tolower ((unsigned char) word[i]);
        if (c == 'y') // a consonant at the start or between vowels (yes, swaying)
            return i > 0 && ! (isV (i - 1) && isV (i + 1));
        return isV (i);
    };
    std::vector<std::pair<size_t, size_t>> groups; // [start, end)
    for (size_t i = 0; i < word.size();)
    {
        if (! vowelAt (i)) { ++i; continue; }
        size_t j = i;
        while (j < word.size() && vowelAt (j)) ++j;
        groups.push_back ({ i, j });
        i = j;
    }
    // a silent final e ("make", "lone", "fire")
    if (groups.size() > 1 && groups.back().first == word.size() - 1 && std::tolower ((unsigned char) word.back()) == 'e')
        groups.pop_back();
    return groups;
}

std::vector<std::string> letterSplit (const std::string& word, size_t n)
{
    if (n >= 2 && word.size() > 4 && toLower (word.substr (word.size() - 3)) == "ing")
    {
        auto v = letterSplit (word.substr (0, word.size() - 3), n - 1);
        v.push_back (word.substr (word.size() - 3));
        return v;
    }
    if (n <= 1 || word.size() < n) 
    {
        std::vector<std::string> v (std::max<size_t> (n, 1));
        v[0] = word;
        return v;
    }
    auto groups = vowelGroups (word);
    if (groups.size() == n + 1 && groups.back().second == word.size()
        && std::tolower ((unsigned char) word.back()) == 'e' && groups.back().second - groups.back().first == 1)
        groups.pop_back();
    // fewer vowel groups than syllables ("media", "vienna"): split the longest groups
    while (groups.size() < n)
    {
        auto it = std::max_element (groups.begin(), groups.end(), [] (auto& a, auto& b) {
            return a.second - a.first < b.second - b.first;
        });
        if (it == groups.end() || it->second - it->first < 2) break;
        const auto g = *it;
        *it = { g.first, g.first + 1 };
        groups.insert (it + 1, { g.first + 1, g.second });
    }

    std::vector<size_t> cuts;
    if (groups.size() == n)
    {
        for (size_t k = 1; k < n; ++k)
        {
            const size_t a = groups[k - 1].second, b = groups[k].first; // consonants in [a, b)
            size_t cut = b - a <= 1 ? a : a + 1;
            if (b - a >= 2) // keep th, sh, ch, ph, wh, gh, ck, ng together
            {
                const std::string pair = toLower (word.substr (a, 2));
                for (const char* d : { "th", "sh", "ch", "ph", "wh", "gh", "ck", "ng" })
                    if (pair == d) cut = b - a == 2 ? a : a + 2;
            }
            cuts.push_back (cut);
        }
    }
    else
        for (size_t k = 1; k < n; ++k)
            cuts.push_back ((word.size() * k + n / 2) / n);

    std::vector<std::string> out;
    size_t s = 0;
    for (size_t c : cuts) { out.push_back (word.substr (s, c - s)); s = c; }
    out.push_back (word.substr (s));
    return out;
}

// Syllabify a flat phoneme list and append it as one word.
void emitWord (const std::vector<int>& idsIn, const std::string& display, int wordIndex, std::vector<Syllable>& out)
{
    // "our", "hour", "fire": the dictionary says ow-er, but it's sung as one note
    std::vector<int> ids = idsIn;
    const size_t spelled = vowelGroups (display).size();
    size_t sung = (size_t) std::count_if (ids.begin(), ids.end(), [] (int id) { return isVowel (id); });
    const int er = ph ("er"), r = ph ("r");
    for (size_t i = 1; i < ids.size() && sung > std::max<size_t> (spelled, 1); ++i)
    {
        const std::string prev = phoneme (ids[i - 1]).name;
        if (ids[i] == er && (prev == "aw" || prev == "ay" || prev == "oy"))
        {
            ids[i] = r;
            --sung;
        }
    }
    std::vector<Ph> phs;
    for (int id : ids) phs.push_back ({ id, 0 });
    auto raws = autoSyllabify (phs);
    auto labels = letterSplit (display, raws.size());
    for (size_t k = 0; k < raws.size(); ++k)
    {
        Syllable s = toSyllable (raws[k].phs);
        s.label = k < labels.size() ? labels[k] : std::string();
        s.word = wordIndex;
        out.push_back (s);
    }
}

void bracketToSyllables (const std::string& inside, int wordIndex, std::vector<Syllable>& out);
bool isPhonemeText (const std::string& s);

// Convert one word token (may contain hyphens as syllable hints) into syllables.
using UserDict = std::unordered_map<std::string, std::string>; // word -> phonemes or respelling

void wordToSyllables (const std::string& token, int wordIndex, std::vector<Syllable>& out, bool spellAcronyms = true,
                      const UserDict* user = nullptr)
{
    const std::string lower = toLower (token);
    std::string joined;
    for (char c : lower) if (c != '-') joined += c;

    // the singer's own definitions ("ceiling = s iy | l ih ng" or "taxicab = tack-see-cab")
    if (user != nullptr)
        if (auto it = user->find (joined); it != user->end())
        {
            std::string rhs = it->second;
            std::vector<Syllable> tmp;
            const bool bracketed = ! rhs.empty() && rhs.front() == '[';
            if (bracketed)
            {
                rhs.erase (0, 1);
                if (! rhs.empty() && rhs.back() == ']') rhs.pop_back();
            }
            if (bracketed || isPhonemeText (rhs))
                bracketToSyllables (rhs, wordIndex, tmp);
            else
            {
                std::istringstream is (rhs);
                std::string w;
                while (is >> w)
                    wordToSyllables (w, wordIndex, tmp, false, nullptr);
            }
            auto labels = letterSplit (joined, tmp.size());
            for (size_t k = 0; k < tmp.size(); ++k)
            {
                tmp[k].label = k < labels.size() ? labels[k] : std::string();
                tmp[k].word = wordIndex;
                out.push_back (tmp[k]);
            }
            if (! tmp.empty()) return;
        }

    if (spellAcronyms && isAcronym (token))
    {
        // acronyms the dictionary says as words (NASA, CERT, PHP) use that
        std::vector<int> ids;
        if (token.size() >= 3 && cmuLookup (joined, ids))
        {
            emitWord (ids, token, wordIndex, out);
            return;
        }
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

    if (lower.find ('-') == std::string::npos)
    {
        std::vector<int> ids;
        if (lookupWord (joined, ids))
        {
            emitWord (ids, joined, wordIndex, out);
            return;
        }
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
    if (! hintBreaks.empty())
    {
        std::vector<std::string> parts;
        for (auto& part : splitOn (lower, '-'))
            if (! part.empty()) parts.push_back (part);

        // a real word with syllable hints ("tax-i-cab"): dictionary sounds, the user's splits
        std::vector<int> ids;
        if (lookupWord (joined, ids))
        {
            std::vector<Ph> p;
            for (int id : ids) p.push_back ({ id, 0 });
            auto raws = autoSyllabify (p);
            if (raws.size() == parts.size())
            {
                for (size_t k = 0; k < raws.size(); ++k)
                {
                    Syllable s = toSyllable (raws[k].phs);
                    s.label = parts[k];
                    s.word = wordIndex;
                    out.push_back (s);
                }
                return;
            }
        }
        // a compound ("sea-breakers", "off-line", "self-hosted"): each part on its own
        if (parts.size() > 1 && std::all_of (parts.begin(), parts.end(), [] (const std::string& x) { return x.size() >= 3; }))
        {
            for (auto& part : parts)
                wordToSyllables (part, wordIndex, out, false, user);
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

// "s iy | l ih ng": every piece is a phoneme name
bool isPhonemeText (const std::string& s)
{
    std::string t = s;
    for (auto& c : t) if (c == '|' || c == '-' || c == '/') c = ' ';
    std::istringstream is (t);
    std::string tok;
    int n = 0;
    while (is >> tok)
    {
        if (findPhoneme (toLower (tok)) < 0) return false;
        ++n;
    }
    return n > 0;
}

// "word = sounds" (one word, then '=', then phonemes, [phonemes] or a respelling).
std::optional<std::pair<std::string, std::string>> parseDefinition (const std::string& line)
{
    const size_t eq = line.find ('=');
    if (eq == std::string::npos) return std::nullopt;
    auto trim = [] (std::string x) {
        const char* ws = " \t\r";
        x.erase (0, x.find_first_not_of (ws));
        const size_t e = x.find_last_not_of (ws);
        x.erase (e == std::string::npos ? 0 : e + 1);
        return x;
    };
    std::string word = trim (line.substr (0, eq)), rhs = trim (line.substr (eq + 1));
    if (word.empty() || rhs.empty()) return std::nullopt;
    std::string key;
    for (char c : word)
    {
        if (std::isalpha ((unsigned char) c) || c == '\'') key += (char) std::tolower ((unsigned char) c);
        else if (c != '-') return std::nullopt; // must be a single word
    }
    if (key.empty()) return std::nullopt;
    return std::make_pair (key, rhs);
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

Lyrics parseLyrics (const std::string& rawText, bool spellAcronyms)
{
    Lyrics L;
    // Lines like "ceiling = s iy | l ih ng" define how a word is said. They
    // apply to the whole lyric and are not sung.
    UserDict user;
    std::string text;
    {
        std::istringstream is (rawText);
        std::string line;
        bool first = true;
        while (std::getline (is, line))
        {
            if (auto def = parseDefinition (line))
            {
                user[def->first] = def->second;
                continue;
            }
            if (! first) text += '\n';
            text += line;
            first = false;
        }
        if (! rawText.empty() && rawText.back() == '\n' && ! text.empty()) text += '\n';
    }
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
                        if (! letters.empty()) { L.words.push_back (letters); wordToSyllables (letters, (int) L.words.size() - 1, L.syllables, spellAcronyms, &user); letters.clear(); }
                        L.words.push_back (digitWord (ch));
                        wordToSyllables (digitWord (ch), (int) L.words.size() - 1, L.syllables);
                    }
                    else letters += ch;
                }
                if (! letters.empty()) { L.words.push_back (letters); wordToSyllables (letters, (int) L.words.size() - 1, L.syllables, spellAcronyms, &user); }
            }
            else if (! tok.empty())
            {
                L.words.push_back (tok);
                wordToSyllables (tok, (int) L.words.size() - 1, L.syllables, spellAcronyms, &user);
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
