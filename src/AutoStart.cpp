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

#include "AutoStart.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

// ── macOS: LaunchAgent plist ──────────────────────────────────────────────────
#if defined(Q_OS_MACOS)

bool AutoStart::isSupported() { return true; }

static QString plistPath() {
    return QDir::homePath() + "/Library/LaunchAgents/ua.ss-education.stepan.plist";
}

bool AutoStart::isEnabled() {
    return QFile::exists(plistPath());
}

bool AutoStart::setEnabled(bool enable) {
    if (!enable)
        return QFile::remove(plistPath());

    QDir().mkpath(QDir::homePath() + "/Library/LaunchAgents");
    QFile f(plistPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\"\n";
    out << "  \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    out << "<plist version=\"1.0\">\n";
    out << "<dict>\n";
    out << "    <key>Label</key>\n";
    out << "    <string>ua.ss-education.stepan</string>\n";
    out << "    <key>ProgramArguments</key>\n";
    out << "    <array>\n";
    out << "        <string>" << QCoreApplication::applicationFilePath() << "</string>\n";
    out << "    </array>\n";
    out << "    <key>RunAtLoad</key>\n";
    out << "    <true/>\n";
    out << "    <key>ProcessType</key>\n";
    out << "    <string>Interactive</string>\n";
    out << "</dict>\n";
    out << "</plist>\n";
    return true;
}

// ── Windows: HKCU Run registry ────────────────────────────────────────────────
#elif defined(Q_OS_WIN)
#include <QSettings>

bool AutoStart::isSupported() { return true; }

static const char* kRegPath =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool AutoStart::isEnabled() {
    return QSettings(kRegPath, QSettings::NativeFormat).contains("Stepan");
}

bool AutoStart::setEnabled(bool enable) {
    QSettings reg(kRegPath, QSettings::NativeFormat);
    if (enable)
        reg.setValue("Stepan",
            "\"" + QCoreApplication::applicationFilePath().replace('/', '\\') + "\"");
    else
        reg.remove("Stepan");
    return true;
}

// ── Linux: XDG autostart .desktop ─────────────────────────────────────────────
#elif defined(Q_OS_LINUX)

bool AutoStart::isSupported() { return true; }

// Path of the program to launch. Inside an AppImage applicationFilePath()
// is the temporary mount (/tmp/.mount_XXXX/...), gone after reboot; the
// AppImage runtime exports the real file path in $APPIMAGE.
static QString executablePath() {
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    if (!appImage.isEmpty() && QFile::exists(appImage))
        return appImage;
    return QCoreApplication::applicationFilePath();
}

// Quote a path for the Exec key (Desktop Entry spec, "The Exec key"):
// inside double quotes, " ` $ \ are backslash-escaped; then the string
// value escaping doubles every backslash once more; % becomes %%.
static QString quoteExec(const QString& path) {
    QString q;
    for (const QChar c : path) {
        if (c == '"' || c == '`' || c == '$') q += "\\\\";
        else if (c == '\\') q += "\\\\\\";
        if (c == '%') q += '%';
        q += c;
    }
    return '"' + q + '"';
}

static QString desktopPath() {
    return QDir::homePath() + "/.config/autostart/stepan.desktop";
}

bool AutoStart::isEnabled() {
    return QFile::exists(desktopPath());
}

bool AutoStart::setEnabled(bool enable) {
    if (!enable)
        return QFile::remove(desktopPath());

    QDir().mkpath(QDir::homePath() + "/.config/autostart");
    QFile f(desktopPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=Stepan\n";
    out << "Comment=Конвертер систем координат\n";
    out << "Exec=" << quoteExec(executablePath()) << "\n";
    out << "Icon=stepan\n";
    out << "Hidden=false\n";
    out << "NoDisplay=false\n";
    out << "X-GNOME-Autostart-enabled=true\n";
    return true;
}

// ── Unsupported ───────────────────────────────────────────────────────────────
#else

bool AutoStart::isSupported() { return false; }
bool AutoStart::isEnabled()   { return false; }
bool AutoStart::setEnabled(bool) { return false; }

#endif
