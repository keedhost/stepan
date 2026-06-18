#pragma once
#include <QString>

// Must be called once in main() after QApplication + org/app name are set.
// Returns a reference to the mutable flag so main() can set it.
inline bool& appLangIsEnglish() {
    static bool s_isEn = false;
    return s_isEn;
}

// Translate: returns English string when language == "en", otherwise Ukrainian.
// Usage: appTr("Рядок українською", "English string")
inline QString appTr(const char* uk, const char* en) {
    return appLangIsEnglish() ? QString::fromUtf8(en) : QString::fromUtf8(uk);
}
