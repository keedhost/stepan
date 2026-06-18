#include "MainWindow.h"
#include "TrayManager.h"
#include "CoordinateParser.h"
#include "AppLocale.h"
#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QIcon>
#include <QList>
#include <QPair>
#include <QSettings>

// ── Helpers ───────────────────────────────────────────────────────────────────

// Strip human-friendly decorators, e.g. "37U DP 11286 19318" → "37UDP1128619318"
static QString toRaw(const QString& key, const QString& v, double lat = 0.0, double lon = 0.0) {
    if (key == "mgrs")    return QString(v).remove(' ');
    if (key == "dd")      return QString("%1,%2").arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6);
    if (key == "dms")     return QString(v).replace("N ", "N").replace("S ", "S");
    if (key == "utm")     return QString(v).replace(' ', ',');
    if (key == "usk2000") return QString(v).remove('-');
    return v;
}

// Generate map service URL(s). Returns {label, url} pairs.
static QList<QPair<QString,QString>> makeUrls(const QString& svc, double lat, double lon) {
    const QString latS = QString::number(lat, 'f', 6);
    const QString lonS = QString::number(lon, 'f', 6);

    const QPair<QString,QString> google = {
        "Google Maps",
        "https://www.google.com/maps/search/?api=1&query=" + latS + "," + lonS
    };
    const QPair<QString,QString> osm = {
        "OpenStreetMap",
        "https://www.openstreetmap.org/?mlat=" + latS + "&mlon=" + lonS + "&zoom=14"
    };
    const QPair<QString,QString> bing = {
        "Bing Maps",
        "https://www.bing.com/maps/?cp=" + latS + "~" + lonS + "&lvl=14"
    };
    if (svc == "google") return {google};
    if (svc == "osm")    return {osm};
    if (svc == "bing")   return {bing};
    if (svc == "all")    return {google, osm, bing};
    return {};
}

// ── Console-only mode ─────────────────────────────────────────────────────────

static int runConsole(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("Stepan");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser p;
    p.setApplicationDescription(
        "Конвертер систем координат (MGRS, DD, DMS, UTM, УСК-2000)");
    p.addHelpOption();
    p.addVersionOption();

    QCommandLineOption convertOpt({"c", "convert"},
        "Конвертувати <координату> та вивести результат.", "координата");
    QCommandLineOption formatOpt({"f", "format"},
        "Вивести лише вказаний формат: mgrs | dd | dms | utm | usk2000.", "формат");
    QCommandLineOption rawOpt({"r", "raw"},
        "Вивести значення без форматування (без пробілів та спеціальних символів).");
    QCommandLineOption urlOpt({"u", "url"},
        "Генерувати посилання на карту: google | osm | bing | all.", "сервіс");

    p.addOption(convertOpt);
    p.addOption(formatOpt);
    p.addOption(rawOpt);
    p.addOption(urlOpt);
    p.process(app);

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (!p.isSet(convertOpt)) {
        err << "Помилка: вкажіть --convert <координата>\n";
        err << "Запустіть з --help для довідки.\n";
        return 1;
    }

    const auto result = CoordinateParser::parse(p.value(convertOpt));
    if (!result) {
        err << "Помилка: формат не розпізнано: " << p.value(convertOpt) << "\n";
        return 2;
    }

    const QString fmtFilter = p.value(formatOpt).toLower();
    const QString urlSvc    = p.value(urlOpt).toLower();
    const bool    useRaw    = p.isSet(rawOpt);
    const bool    hasUrl    = p.isSet(urlOpt);
    const bool    hasFormat = p.isSet(formatOpt);

    // Validate --url service
    if (hasUrl && !QStringList{"google","osm","bing","all"}.contains(urlSvc)) {
        err << "Невідомий сервіс: " << urlSvc
            << "  (google | osm | bing | all)\n";
        return 3;
    }

    // Coordinate output: skip when --url is the only output-related flag
    const bool showCoords = !hasUrl || hasFormat;

    if (showCoords) {
        struct Entry { QString key, label, value; };
        const QList<Entry> all = {
            {"mgrs",    "MGRS:     ", result->mgrs},
            {"dd",      "DD:       ", result->dd},
            {"dms",     "DMS:      ", result->dms},
            {"utm",     "UTM:      ", result->utm},
            {"usk2000", "УСК-2000: ", result->usk2000},
        };

        if (!fmtFilter.isEmpty()) {
            // Single format
            bool found = false;
            for (const auto& e : all) {
                if (e.key == fmtFilter) {
                    const QString val = useRaw
                        ? toRaw(e.key, e.value, result->lat, result->lon)
                        : e.value;
                    out << val << "\n";
                    found = true;
                    break;
                }
            }
            if (!found) {
                err << "Невідомий формат: " << fmtFilter
                    << "  (mgrs | dd | dms | utm | usk2000)\n";
                return 3;
            }
        } else {
            // All formats
            for (const auto& e : all) {
                const QString val = useRaw
                    ? toRaw(e.key, e.value, result->lat, result->lon)
                    : e.value;
                out << e.label << val << "\n";
            }
        }
    }

    // URL output
    if (hasUrl) {
        const auto urls = makeUrls(urlSvc, result->lat, result->lon);
        if (showCoords && !urls.isEmpty()) out << "\n";
        if (urls.size() == 1) {
            out << urls.first().second << "\n";
        } else {
            int maxLabel = 0;
            for (const auto& u : urls) maxLabel = qMax(maxLabel, u.first.size());
            for (const auto& u : urls)
                out << u.first.leftJustified(maxLabel + 2) << u.second << "\n";
        }
    }

    return 0;
}

// ── Detect whether any arg triggers console-only mode ─────────────────────────

static bool isConsoleMode(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const QLatin1String a(argv[i]);
        if (a == "--convert" || a == "-c" ||
            a == "--help"    || a == "-h" ||
            a == "--version" || a == "-v" ||
            a == "--url"     || a == "-u" ||
            a == "--raw"     || a == "-r")
            return true;
    }
    return false;
}

// ── GUI mode ──────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (isConsoleMode(argc, argv))
        return runConsole(argc, argv);

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);
    app.setApplicationName("Stepan");
    app.setApplicationDisplayName("Степан");
    app.setOrganizationName("ss-education");
    app.setOrganizationDomain("ss-education.com.ua");
    app.setApplicationVersion("1.0.0");

    appLangIsEnglish() = QSettings().value("language", "uk").toString() == "en";

    QIcon appIcon;
    appIcon.addFile(":/icons/color/16.png",  {16, 16});
    appIcon.addFile(":/icons/color/24.png",  {24, 24});
    appIcon.addFile(":/icons/color/32.png",  {32, 32});
    appIcon.addFile(":/icons/color/64.png",  {64, 64});
    appIcon.addFile(":/icons/color/128.png", {128, 128});
    appIcon.addFile(":/icons/color/256.png", {256, 256});
    appIcon.addFile(":/icons/color/512.png", {512, 512});
    app.setWindowIcon(appIcon);

    app.setQuitOnLastWindowClosed(false);

    // Parse --open <coord> or bare positional coordinate
    QCommandLineParser clp;
    clp.addOption({{"o", "open"}, "Відкрити GUI з координатою.", "координата"});
    clp.addPositionalArgument("координата", "Відкрити GUI з цією координатою.");
    clp.parse(app.arguments());

    MainWindow window;

    const QString initial = clp.isSet("open")
        ? clp.value("open")
        : (clp.positionalArguments().isEmpty()
               ? QString()
               : clp.positionalArguments().first());

    if (!initial.isEmpty()) {
        window.setInputText(initial);
        window.show(); // always show when coordinate provided via CLI
    } else if (!QSettings().value("startMinimized", false).toBool()) {
        window.show();
    }

    TrayManager tray;

    QObject::connect(&tray, &TrayManager::coordinateEntered,
                     [&window](const QString& text) {
        window.setInputText(text);
        window.show();
        window.raise();
        window.activateWindow();
    });

    QObject::connect(&tray, &TrayManager::showWindowRequested,
                     [&window]() {
        window.show();
        window.raise();
        window.activateWindow();
    });

    QObject::connect(&tray, &TrayManager::quitRequested,
                     &app, &QApplication::quit);

    QObject::connect(&window, &MainWindow::settingsChanged,
                     &tray,   &TrayManager::reloadTrayIcon);

    return app.exec();
}
