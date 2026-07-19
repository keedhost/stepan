#pragma once
#include <QApplication>
#include <QPalette>
#include <QString>

// Central dark/light color source for widgets and dialogs that build their
// own inline stylesheets or HTML content instead of relying on Qt's native
// palette cascade (QDialogs don't inherit MainWindow's setStyleSheet(), and
// QTextBrowser HTML needs explicit colors either way).
namespace Theme {

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
inline QString segmentedButtonQss(const QString& selector) {
    const Colors c = colors();
    return QString(
        "%1{ padding:5px 18px; font-size:12px; border:1px solid %2; background:%3; color:%4; }"
        "%1:checked{ background:%5; color:%6; border-color:%5; }"
        "%1:hover:!checked{ background:%7; }"
    ).arg(selector, c.border, c.buttonBg, c.text, c.accent, c.accentText, c.hoverBg);
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
