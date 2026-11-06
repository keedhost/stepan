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
#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>
#include <QString>
#include <QStringList>

// Central dark/light color source for widgets and dialogs that build their
// own inline stylesheets or HTML content instead of relying on Qt's native
// palette cascade (QDialogs don't inherit MainWindow's setStyleSheet(), and
// QTextBrowser HTML needs explicit colors either way).
namespace Theme {

// User-selected theme: "system" (follow OS), "light" or "dark". Stored in
// QSettings under "theme". Forcing a scheme (QStyleHints on Qt >= 6.8, an
// explicit app palette on older Qt) switches the application palette, which fires QEvent::PaletteChange on widgets — MainWindow
// already rebuilds its stylesheet on that, so the change applies live.
inline QString savedMode() {
    return QSettings().value("theme", "system").toString();
}

// Explicit light/dark application palette, used to force a theme on Qt
// versions without QStyleHints::setColorScheme() (added in Qt 6.8; CI builds
// against Qt 6.7 on Windows/macOS and Ubuntu's Qt 6.4 on Linux).
inline QPalette forcedPalette(bool dark) {
    struct Tones { const char *window, *text, *base, *altBase, *button, *mid, *disabled,
                                *placeholder, *link, *bright; };
    const Tones v = dark
        ? Tones{"#2b2b2b", "#e8e8e8", "#3a3a3a", "#323232", "#3c3c3c",
                "#5a5a5a", "#808080", "#a0a0a0", "#7db2ff", "#ff7a7a"}
        : Tones{"#ececec", "#1a1a1a", "#ffffff", "#f5f5f5", "#f0f0f0",
                "#c8c8c8", "#9a9a9a", "#666666", "#2a5a9a", "#c0392b"};
    QPalette p;
    p.setColor(QPalette::Window,          QColor(v.window));
    p.setColor(QPalette::WindowText,      QColor(v.text));
    p.setColor(QPalette::Base,            QColor(v.base));
    p.setColor(QPalette::AlternateBase,   QColor(v.altBase));
    p.setColor(QPalette::Text,            QColor(v.text));
    p.setColor(QPalette::Button,          QColor(v.button));
    p.setColor(QPalette::ButtonText,      QColor(v.text));
    p.setColor(QPalette::ToolTipBase,     QColor(v.button));
    p.setColor(QPalette::ToolTipText,     QColor(v.text));
    p.setColor(QPalette::PlaceholderText, QColor(v.placeholder));
    p.setColor(QPalette::Link,            QColor(v.link));
    p.setColor(QPalette::BrightText,      QColor(v.bright));
    p.setColor(QPalette::Highlight,       QColor("#3d7cf5"));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::Mid,             QColor(v.mid));
    p.setColor(QPalette::Light,           QColor(v.button).lighter(120));
    p.setColor(QPalette::Midlight,        QColor(v.button).lighter(110));
    p.setColor(QPalette::Dark,            QColor(v.mid).darker(120));
    p.setColor(QPalette::Shadow,          QColor("#000000"));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setColor(QPalette::Disabled, role, QColor(v.disabled));
    return p;
}

// Palette-based fallback for applyMode(). A default-constructed QPalette has
// an empty resolve mask, so setting it hands control back to the platform
// palette (which keeps following OS light/dark changes) for "system".
inline void applyModeViaPalette(const QString& mode) {
    if (mode == "light" || mode == "dark")
        QApplication::setPalette(forcedPalette(mode == "dark"));
    else
        QApplication::setPalette(QPalette());
}

inline void applyMode(const QString& mode) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    auto* hints = QGuiApplication::styleHints();
    if (mode == "light")      hints->setColorScheme(Qt::ColorScheme::Light);
    else if (mode == "dark")  hints->setColorScheme(Qt::ColorScheme::Dark);
    else                      hints->unsetColorScheme();
#else
    applyModeViaPalette(mode);
#endif
}

inline bool isDark() {
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

struct Colors {
    QString bg;          // window/dialog background
    QString panelBg;      // card/frame background (slightly offset from bg)
    QString text;         // primary text
    QString textMuted;    // secondary/hint text
    QString border;
    QString link;
    QString danger;       // errors, over-limit warnings
    QString success;
    QString warning;
    QString accent;       // buttons, selection, checked state
    QString accentText;   // text on top of `accent`
    QString accentHover;  // accent shade for :hover/:pressed
    QString inputBg;
    QString hoverBg;
    QString buttonBg;     // default (non-accent) button background
    QString pressedBg;    // default button background while pressed
    QString dangerBg;     // subtle background tint for error highlighting
    QString successBg;    // subtle background tint for success banners
    QString successBorder;
    QString codeComment;  // syntax-highlight color for "# comment" lines in code samples
};

inline Colors colors() {
    if (isDark()) {
        return Colors{
            /*bg*/           "#2b2b2b",
            /*panelBg*/      "#242424",
            /*text*/         "#e8e8e8",
            /*textMuted*/    "#a0a0a0",
            /*border*/       "#5a5a5a",
            /*link*/         "#7db2ff",
            /*danger*/       "#ff7a7a",
            /*success*/      "#7adc7a",
            /*warning*/      "#e0b050",
            /*accent*/       "#3d7cf5",
            /*accentText*/   "#ffffff",
            /*accentHover*/  "#5a94ff",
            /*inputBg*/      "#3a3a3a",
            /*hoverBg*/      "#3c3c3c",
            /*buttonBg*/     "#3c3c3c",
            /*pressedBg*/    "#5a5a5a",
            /*dangerBg*/     "#5a2a2a",
            /*successBg*/    "#1a3a1a",
            /*successBorder*/"#2a6a2a",
            /*codeComment*/  "#8fbc7a",
        };
    }
    return Colors{
        /*bg*/           "#ffffff",
        /*panelBg*/      "#f5f5f5",
        /*text*/         "#1a1a1a",
        /*textMuted*/    "#666666",
        /*border*/       "#c8c8c8",
        /*link*/         "#2a5a9a",
        /*danger*/       "#c0392b",
        /*success*/      "#1a6a1a",
        /*warning*/      "#a06000",
        /*accent*/       "#3d7cf5",
        /*accentText*/   "#ffffff",
        /*accentHover*/  "#2a5fd0",
        /*inputBg*/      "#ffffff",
        /*hoverBg*/      "#e8f0fe",
        /*buttonBg*/     "#f0f0f0",
        /*pressedBg*/    "#c6d5f9",
        /*dangerBg*/     "#ffe0e0",
        /*successBg*/    "#e8f4e8",
        /*successBorder*/"#9acd9a",
        /*codeComment*/  "#6a9955",
    };
}

// Reusable QSS for a segmented toggle-button group (radio-like QPushButtons
// with setCheckable(true)). `selector` scopes the rules — pass an objectName
// selector (e.g. "QPushButton#mapBtnFirst") or a bare type selector
// ("QPushButton") if the group isn't otherwise objectName-scoped.
// Appends a pseudo-state to every part of a comma-separated selector list:
// "A,B" + ":checked" -> "A:checked,B:checked" (naive "%1:checked" would only
// apply the state to the last part and style the rest unconditionally).
inline QString withState(const QString& selector, const QString& state) {
    QStringList parts = selector.split(',');
    for (QString& p : parts) p = p.trimmed() + state;
    return parts.join(',');
}

inline QString segmentedButtonQss(const QString& selector) {
    const Colors c = colors();
    return QString(
        "%1{ padding:5px 18px; font-size:12px; border:1px solid %2; background:%3; color:%4; }"
        "%8{ background:%5; color:%6; border-color:%5; }"
        "%9{ background:%7; color:%4; }"
    ).arg(selector, c.border, c.buttonBg, c.text, c.accent, c.accentText, c.hoverBg,
          withState(selector, ":checked"), withState(selector, ":hover:!checked"));
}

// Reusable QSS for a QTabWidget's tab bar. The native macOS tab style draws
// the selected tab label in white regardless of the actual background, which
// is unreadable on the light theme — so tabs get explicit theme colors.
inline QString tabWidgetQss() {
    const Colors c = colors();
    return QString(
        "QTabWidget::pane{ border:1px solid %1; border-radius:4px; top:-1px; }"
        "QTabWidget::tab-bar{ alignment:center; }"
        // font-size goes on QTabBar itself, not on ::tab: QTabBar measures tab
        // widths with its own font, so a font set only on ::tab is drawn
        // larger than it was measured and the labels get elided ("Загал...").
        "QTabBar{ font-size:12px; }"
        "QTabBar::tab{ padding:5px 16px; color:%2; background:%3;"
        "    border:1px solid %1; border-bottom:none; margin-right:-1px; }"
        "QTabBar::tab:first{ border-top-left-radius:4px; }"
        "QTabBar::tab:last{ border-top-right-radius:4px; margin-right:0; }"
        "QTabBar::tab:selected{ background:%4; color:%2; border-bottom:2px solid %5; }"
        "QTabBar::tab:hover:!selected{ background:%6; }"
    ).arg(c.border, c.text, c.buttonBg, c.bg, c.accent, c.hoverBg);
}

// Reusable QSS for a filled accent "primary action" QPushButton.
inline QString primaryButtonQss(const QString& selector) {
    const Colors c = colors();
    return QString(
        "%1{ padding:7px 16px; border:1px solid %2; border-radius:5px;"
        "    background:%2; color:%3; font-size:13px; font-weight:bold; }"
        "%1:hover{ background:%4; border-color:%4; }"
        "%1:disabled{ background:%5; border-color:%5; color:%3; }"
    ).arg(selector, c.accent, c.accentText, c.accentHover, c.border);
}

// Reusable QSS for an outlined "secondary action" QPushButton (accent color
// configurable so it can read as e.g. success-green or plain neutral).
inline QString outlineButtonQss(const QString& selector, const QString& accentColor,
                                 const QString& hoverBg) {
    const Colors c = colors();
    return QString(
        "%1{ padding:7px 16px; border:1px solid %2; border-radius:5px;"
        "    background:%3; color:%2; font-size:13px; }"
        "%1:hover{ background:%4; }"
        "%1:disabled{ color:%5; border-color:%5; }"
    ).arg(selector, accentColor, c.bg, hoverBg, c.textMuted);
}

// Reusable QSS for a plain neutral QPushButton (Close/Cancel-style).
inline QString neutralButtonQss(const QString& selector) {
    const Colors c = colors();
    return QString(
        "%1{ padding:7px 16px; border:1px solid %2; border-radius:5px;"
        "    background:%3; color:%4; font-size:13px; }"
        "%1:hover{ background:%5; }"
    ).arg(selector, c.border, c.panelBg, c.text, c.hoverBg);
}

// Small CSS block for the <body> tag of QTextBrowser HTML content, so the
// text pane's background/text actually matches the surrounding dialog
// instead of a hardcoded white card.
inline QString htmlBodyStyle() {
    const Colors c = colors();
    return QString("color:%1; background:%2;").arg(c.text, c.bg);
}

} // namespace Theme
