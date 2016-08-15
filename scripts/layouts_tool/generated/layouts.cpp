#include <iostream>

struct KbdKeyChar {
    char16_t character;
    bool is_dead;
};

typedef const KbdKeyChar (*KeyboardLayoutPointer)[2];


// English (US)
static const KbdKeyChar en_US[48][2] = {
        // Digits row
        /* <TLDE> */ {{'`'}, {'~'}},
        /* <AE01> */ {{'1'}, {'!'}},
        /* <AE02> */ {{'2'}, {'@'}},
        /* <AE03> */ {{'3'}, {'#'}},
        /* <AE04> */ {{'4'}, {'$'}},
        /* <AE05> */ {{'5'}, {'%'}},
        /* <AE06> */ {{'6'}, {'^'}},
        /* <AE07> */ {{'7'}, {'&'}},
        /* <AE08> */ {{'8'}, {'*'}},
        /* <AE09> */ {{'9'}, {'('}},
        /* <AE10> */ {{'0'}, {')'}},
        /* <AE11> */ {{'-'}, {'_'}},
        /* <AE12> */ {{'='}, {'+'}},

        // Upper row
        /* <AD01> */ {{'q'}, {'Q'}},
        /* <AD02> */ {{'w'}, {'W'}},
        /* <AD03> */ {{'e'}, {'E'}},
        /* <AD04> */ {{'r'}, {'R'}},
        /* <AD05> */ {{'t'}, {'T'}},
        /* <AD06> */ {{'y'}, {'Y'}},
        /* <AD07> */ {{'u'}, {'U'}},
        /* <AD08> */ {{'i'}, {'I'}},
        /* <AD09> */ {{'o'}, {'O'}},
        /* <AD10> */ {{'p'}, {'P'}},
        /* <AD11> */ {{'['}, {'{'}},
        /* <AD12> */ {{']'}, {'}'}},

        // Home row
        /* <AC01> */ {{'a'}, {'A'}},
        /* <AC02> */ {{'s'}, {'S'}},
        /* <AC03> */ {{'d'}, {'D'}},
        /* <AC04> */ {{'f'}, {'F'}},
        /* <AC05> */ {{'g'}, {'G'}},
        /* <AC06> */ {{'h'}, {'H'}},
        /* <AC07> */ {{'j'}, {'J'}},
        /* <AC08> */ {{'k'}, {'K'}},
        /* <AC09> */ {{'l'}, {'L'}},
        /* <AC10> */ {{';'}, {':'}},
        /* <AC11> */ {{'\''}, {'\"'}},
        /* <BKSL> */ {{'\\'}, {'|'}},

        // Lower row
        /* <LSGT> */ {{'<'}, {'>'}},
        /* <AB01> */ {{'z'}, {'Z'}},
        /* <AB02> */ {{'x'}, {'X'}},
        /* <AB03> */ {{'c'}, {'C'}},
        /* <AB04> */ {{'v'}, {'V'}},
        /* <AB05> */ {{'b'}, {'B'}},
        /* <AB06> */ {{'n'}, {'N'}},
        /* <AB07> */ {{'m'}, {'M'}},
        /* <AB08> */ {{','}, {'<'}},
        /* <AB09> */ {{'.'}, {'>'}},
        /* <AB10> */ {{'/'}, {'?'}}
};

// English (UK)
static const KbdKeyChar en_GB[48][2] = {
        // Digits row
        /* <TLDE> */ {{'`'}, {u'\u00ac'}},
        /* <AE01> */ {{'1'}, {'!'}},
        /* <AE02> */ {{'2'}, {'\"'}},
        /* <AE03> */ {{'3'}, {u'\u00a3'}},
        /* <AE04> */ {{'4'}, {'$'}},
        /* <AE05> */ {{'5'}, {'%'}},
        /* <AE06> */ {{'6'}, {'^'}},
        /* <AE07> */ {{'7'}, {'&'}},
        /* <AE08> */ {{'8'}, {'*'}},
        /* <AE09> */ {{'9'}, {'('}},
        /* <AE10> */ {{'0'}, {')'}},
        /* <AE11> */ {{'-'}, {'_'}},
        /* <AE12> */ {{'='}, {'+'}},

        // Upper row
        /* <AD01> */ {{'q'}, {'Q'}},
        /* <AD02> */ {{'w'}, {'W'}},
        /* <AD03> */ {{'e'}, {'E'}},
        /* <AD04> */ {{'r'}, {'R'}},
        /* <AD05> */ {{'t'}, {'T'}},
        /* <AD06> */ {{'y'}, {'Y'}},
        /* <AD07> */ {{'u'}, {'U'}},
        /* <AD08> */ {{'i'}, {'I'}},
        /* <AD09> */ {{'o'}, {'O'}},
        /* <AD10> */ {{'p'}, {'P'}},
        /* <AD11> */ {{'['}, {'{'}},
        /* <AD12> */ {{']'}, {'}'}},

        // Home row
        /* <AC01> */ {{'a'}, {'A'}},
        /* <AC02> */ {{'s'}, {'S'}},
        /* <AC03> */ {{'d'}, {'D'}},
        /* <AC04> */ {{'f'}, {'F'}},
        /* <AC05> */ {{'g'}, {'G'}},
        /* <AC06> */ {{'h'}, {'H'}},
        /* <AC07> */ {{'j'}, {'J'}},
        /* <AC08> */ {{'k'}, {'K'}},
        /* <AC09> */ {{'l'}, {'L'}},
        /* <AC10> */ {{';'}, {':'}},
        /* <AC11> */ {{'\''}, {'@'}},
        /* <BKSL> */ {{'#'}, {'~'}},

        // Lower row
        /* <LSGT> */ {{'\\'}, {'|'}},
        /* <AB01> */ {{'z'}, {'Z'}},
        /* <AB02> */ {{'x'}, {'X'}},
        /* <AB03> */ {{'c'}, {'C'}},
        /* <AB04> */ {{'v'}, {'V'}},
        /* <AB05> */ {{'b'}, {'B'}},
        /* <AB06> */ {{'n'}, {'N'}},
        /* <AB07> */ {{'m'}, {'M'}},
        /* <AB08> */ {{','}, {'<'}},
        /* <AB09> */ {{'.'}, {'>'}},
        /* <AB10> */ {{'/'}, {'?'}}
};

// Spanish (Spain)
static const KbdKeyChar es_ES[48][2] = {
        // Digits row
        /* <TLDE> */ {{u'\u00ba'}, {u'\u00aa'}},
        /* <AE01> */ {{'1'}, {'!'}},
        /* <AE02> */ {{'2'}, {'\"'}},
        /* <AE03> */ {{'3'}, {u'\u00b7'}},
        /* <AE04> */ {{'4'}, {'$'}},
        /* <AE05> */ {{'5'}, {'%'}},
        /* <AE06> */ {{'6'}, {'&'}},
        /* <AE07> */ {{'7'}, {'/'}},
        /* <AE08> */ {{'8'}, {'('}},
        /* <AE09> */ {{'9'}, {')'}},
        /* <AE10> */ {{'0'}, {'='}},
        /* <AE11> */ {{'\''}, {'?'}},
        /* <AE12> */ {{u'\u00a1'}, {u'\u00bf'}},

        // Upper row
        /* <AD01> */ {{'q'}, {'Q'}},
        /* <AD02> */ {{'w'}, {'W'}},
        /* <AD03> */ {{'e'}, {'E'}},
        /* <AD04> */ {{'r'}, {'R'}},
        /* <AD05> */ {{'t'}, {'T'}},
        /* <AD06> */ {{'y'}, {'Y'}},
        /* <AD07> */ {{'u'}, {'U'}},
        /* <AD08> */ {{'i'}, {'I'}},
        /* <AD09> */ {{'o'}, {'O'}},
        /* <AD10> */ {{'p'}, {'P'}},
        /* <AD11> */ {{u'\u0300', true}, {u'\u0302', true}},
        /* <AD12> */ {{'+'}, {'*'}},

        // Home row
        /* <AC01> */ {{'a'}, {'A'}},
        /* <AC02> */ {{'s'}, {'S'}},
        /* <AC03> */ {{'d'}, {'D'}},
        /* <AC04> */ {{'f'}, {'F'}},
        /* <AC05> */ {{'g'}, {'G'}},
        /* <AC06> */ {{'h'}, {'H'}},
        /* <AC07> */ {{'j'}, {'J'}},
        /* <AC08> */ {{'k'}, {'K'}},
        /* <AC09> */ {{'l'}, {'L'}},
        /* <AC10> */ {{u'\u00f1'}, {u'\u00d1'}},
        /* <AC11> */ {{u'\u0301', true}, {u'\u0308', true}},
        /* <BKSL> */ {{u'\u00e7'}, {u'\u00c7'}},

        // Lower row
        /* <LSGT> */ {{'<'}, {'>'}},
        /* <AB01> */ {{'z'}, {'Z'}},
        /* <AB02> */ {{'x'}, {'X'}},
        /* <AB03> */ {{'c'}, {'C'}},
        /* <AB04> */ {{'v'}, {'V'}},
        /* <AB05> */ {{'b'}, {'B'}},
        /* <AB06> */ {{'n'}, {'N'}},
        /* <AB07> */ {{'m'}, {'M'}},
        /* <AB08> */ {{','}, {';'}},
        /* <AB09> */ {{'.'}, {':'}},
        /* <AB10> */ {{'-'}, {'_'}}
};

// Spanish (Latin America, (Mexico) )
static const KbdKeyChar es_MX[48][2] = {
        // Digits row
        /* <TLDE> */ {{'|'}, {u'\u00b0'}},
        /* <AE01> */ {{'1'}, {'!'}},
        /* <AE02> */ {{'2'}, {'\"'}},
        /* <AE03> */ {{'3'}, {'#'}},
        /* <AE04> */ {{'4'}, {'$'}},
        /* <AE05> */ {{'5'}, {'%'}},
        /* <AE06> */ {{'6'}, {'&'}},
        /* <AE07> */ {{'7'}, {'/'}},
        /* <AE08> */ {{'8'}, {'('}},
        /* <AE09> */ {{'9'}, {')'}},
        /* <AE10> */ {{'0'}, {'='}},
        /* <AE11> */ {{'\''}, {'?'}},
        /* <AE12> */ {{u'\u00bf'}, {u'\u00a1'}},

        // Upper row
        /* <AD01> */ {{'q'}, {'Q'}},
        /* <AD02> */ {{'w'}, {'W'}},
        /* <AD03> */ {{'e'}, {'E'}},
        /* <AD04> */ {{'r'}, {'R'}},
        /* <AD05> */ {{'t'}, {'T'}},
        /* <AD06> */ {{'y'}, {'Y'}},
        /* <AD07> */ {{'u'}, {'U'}},
        /* <AD08> */ {{'i'}, {'I'}},
        /* <AD09> */ {{'o'}, {'O'}},
        /* <AD10> */ {{'p'}, {'P'}},
        /* <AD11> */ {{u'\u0301', true}, {u'\u0308', true}},
        /* <AD12> */ {{'+'}, {'*'}},

        // Home row
        /* <AC01> */ {{'a'}, {'A'}},
        /* <AC02> */ {{'s'}, {'S'}},
        /* <AC03> */ {{'d'}, {'D'}},
        /* <AC04> */ {{'f'}, {'F'}},
        /* <AC05> */ {{'g'}, {'G'}},
        /* <AC06> */ {{'h'}, {'H'}},
        /* <AC07> */ {{'j'}, {'J'}},
        /* <AC08> */ {{'k'}, {'K'}},
        /* <AC09> */ {{'l'}, {'L'}},
        /* <AC10> */ {{u'\u00f1'}, {u'\u00d1'}},
        /* <AC11> */ {{'{'}, {'['}},
        /* <BKSL> */ {{'}'}, {']'}},

        // Lower row
        /* <LSGT> */ {{'<'}, {'>'}},
        /* <AB01> */ {{'z'}, {'Z'}},
        /* <AB02> */ {{'x'}, {'X'}},
        /* <AB03> */ {{'c'}, {'C'}},
        /* <AB04> */ {{'v'}, {'V'}},
        /* <AB05> */ {{'b'}, {'B'}},
        /* <AB06> */ {{'n'}, {'N'}},
        /* <AB07> */ {{'m'}, {'M'}},
        /* <AB08> */ {{','}, {';'}},
        /* <AB09> */ {{'.'}, {':'}},
        /* <AB10> */ {{'-'}, {'_'}}
};

// French (France)
static const KbdKeyChar fr_FR[48][2] = {
        // Digits row
        /* <TLDE> */ {{u'\u00b2'}, {'~'}},
        /* <AE01> */ {{'&'}, {'1'}},
        /* <AE02> */ {{u'\u00e9'}, {'2'}},
        /* <AE03> */ {{'\"'}, {'3'}},
        /* <AE04> */ {{'\''}, {'4'}},
        /* <AE05> */ {{'('}, {'5'}},
        /* <AE06> */ {{'-'}, {'6'}},
        /* <AE07> */ {{u'\u00e8'}, {'7'}},
        /* <AE08> */ {{'_'}, {'8'}},
        /* <AE09> */ {{u'\u00e7'}, {'9'}},
        /* <AE10> */ {{u'\u00e0'}, {'0'}},
        /* <AE11> */ {{')'}, {u'\u00b0'}},
        /* <AE12> */ {{'='}, {'+'}},

        // Upper row
        /* <AD01> */ {{'a'}, {'A'}},
        /* <AD02> */ {{'z'}, {'Z'}},
        /* <AD03> */ {{'e'}, {'E'}},
        /* <AD04> */ {{'r'}, {'R'}},
        /* <AD05> */ {{'t'}, {'T'}},
        /* <AD06> */ {{'y'}, {'Y'}},
        /* <AD07> */ {{'u'}, {'U'}},
        /* <AD08> */ {{'i'}, {'I'}},
        /* <AD09> */ {{'o'}, {'O'}},
        /* <AD10> */ {{'p'}, {'P'}},
        /* <AD11> */ {{u'\u0302', true}, {u'\u0308', true}},
        /* <AD12> */ {{'$'}, {u'\u00a3'}},

        // Home row
        /* <AC01> */ {{'q'}, {'Q'}},
        /* <AC02> */ {{'s'}, {'S'}},
        /* <AC03> */ {{'d'}, {'D'}},
        /* <AC04> */ {{'f'}, {'F'}},
        /* <AC05> */ {{'g'}, {'G'}},
        /* <AC06> */ {{'h'}, {'H'}},
        /* <AC07> */ {{'j'}, {'J'}},
        /* <AC08> */ {{'k'}, {'K'}},
        /* <AC09> */ {{'l'}, {'L'}},
        /* <AC10> */ {{'m'}, {'M'}},
        /* <AC11> */ {{u'\u00f9'}, {'%'}},
        /* <BKSL> */ {{'*'}, {u'\u00b5'}},

        // Lower row
        /* <LSGT> */ {{'<'}, {'>'}},
        /* <AB01> */ {{'w'}, {'W'}},
        /* <AB02> */ {{'x'}, {'X'}},
        /* <AB03> */ {{'c'}, {'C'}},
        /* <AB04> */ {{'v'}, {'V'}},
        /* <AB05> */ {{'b'}, {'B'}},
        /* <AB06> */ {{'n'}, {'N'}},
        /* <AB07> */ {{','}, {'?'}},
        /* <AB08> */ {{';'}, {'.'}},
        /* <AB09> */ {{':'}, {'/'}},
        /* <AB10> */ {{'!'}, {u'\u00a7'}}
};

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */

/* @START GENERATED */
extern "C" KeyboardLayoutPointer getLayout(std::string layoutName) {
    if (layoutName == "en_US") return en_US;
    if (layoutName == "en_GB") return en_GB;
    if (layoutName == "es_ES") return es_ES;
    if (layoutName == "es_MX") return es_MX;
    if (layoutName == "fr_FR") return fr_FR;
    else {
        return nullptr;
    }
}
/* @END GENERATED */
