#pragma once
#include <QString>
#include <QMessageBox>
#include <QPushButton>

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

// QMessageBox's static convenience functions (warning/information/question)
// label their buttons from Qt's own translation catalog (qtbase_*.qm), which
// this app doesn't ship, so "OK"/"Yes"/"No" always show in English regardless
// of the selected app language. These helpers build the box by hand so its
// buttons follow appTr() like every other piece of text in the app.

inline void showAppWarning(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox box(QMessageBox::Warning, title, text, QMessageBox::NoButton, parent);
    box.addButton(appTr("OK", "OK"), QMessageBox::AcceptRole);
    box.exec();
}

inline void showAppInfo(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox box(QMessageBox::Information, title, text, QMessageBox::NoButton, parent);
    box.addButton(appTr("OK", "OK"), QMessageBox::AcceptRole);
    box.exec();
}

// Returns true if the user picked "Так"/"Yes".
inline bool askAppYesNo(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
    auto* yesBtn = box.addButton(appTr("Так", "Yes"), QMessageBox::YesRole);
    box.addButton(appTr("Ні", "No"), QMessageBox::NoRole);
    box.setDefaultButton(yesBtn);
    box.exec();
    return box.clickedButton() == yesBtn;
}
