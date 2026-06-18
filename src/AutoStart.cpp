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
    out << "Exec=" << QCoreApplication::applicationFilePath() << "\n";
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
