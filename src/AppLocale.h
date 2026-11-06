/*
 * Stepan — твій друг в системі координат
 * Copyright (C) 2026 Andrii Kondratiev
 *
 * Слава Україні! Слава ЗСУ!
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
