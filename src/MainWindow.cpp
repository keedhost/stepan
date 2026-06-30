#include "MainWindow.h"
#include "CoordinateParser.h"
#include "MapWidget.h"
#include "SettingsDialog.h"
#include "AboutDialog.h"
#include "WebServer.h"
#include "AppLocale.h"

#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QUrl>
#include <QFont>
#include <QSizePolicy>
#include <QCloseEvent>
#include <QEvent>
#include <QPalette>
#include <QMenuBar>
#include <QMenu>
#include <QDialog>
#include <QTextBrowser>
#include <QTextDocument>
#include <QImage>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>
#include <QCursor>
#include <QPixmap>
#include <QSettings>
#include <QCoreApplication>
#include <QPainter>
#include <QTextCursor>
#include "BulkMarkersDialog.h"

static QString buildStylesheet(bool dark) {
    const char* bd  = dark ? "#5a5a5a" : "#c8c8c8"; // border
    const char* bg  = dark ? "#2a2a2a" : "#ffffff";  // input bg
    const char* fg  = dark ? "#e8e8e8" : "#1a1a1a";  // input text
    const char* bb  = dark ? "#3c3c3c" : "#f0f0f0";  // button bg
    const char* bt  = dark ? "#e8e8e8" : "#1a1a1a";  // button text
    const char* hv  = dark ? "#505050" : "#e8f0fe";  // hover bg
    const char* pr  = dark ? "#5a5a5a" : "#c6d5f9";  // pressed bg
    const char* pb  = dark ? "#242424" : "#f8f8f8";  // panel bg

    return QString(
        "QLineEdit#coordInput{"
        " font-size:14px;padding:8px 12px;border:2px solid %1;"
        " border-radius:6px;background:%2;color:%3;font-family:monospace;}"
        "QLineEdit#coordInput:focus{border-color:#3d7cf5;}"
        "QLabel#formatLabel{font-size:12px;padding:2px 4px;}"
        "QPushButton#externalBtn{"
        " padding:6px 14px;border:1px solid %1;border-radius:5px;"
        " background:%4;color:%5;font-size:12px;}"
        "QPushButton#externalBtn:hover{background:%6;border-color:#3d7cf5;}"
        "QPushButton#externalBtn:pressed{background:%7;}"
        "QPushButton#externalBtn:disabled{color:%1;}"
        "QFrame#coordsPanel{background:%8;border:1px solid %1;border-radius:6px;}"
        "QLabel.coordName{font-weight:bold;font-size:12px;min-width:50px;}"
        "QLabel.coordValue{font-family:monospace;font-size:12px;padding:2px 6px;}"
        "QPushButton#copyBtn{"
        " padding:3px 10px;border:1px solid %1;border-radius:4px;"
        " background:%4;color:%5;font-size:11px;min-width:70px;}"
        "QPushButton#copyBtn:hover{background:%6;border-color:#5a5;}"
        "QPushButton#copyBtn:pressed{background:%7;}"
        "QPushButton#pasteBtn{"
        " padding:8px 14px;border:2px solid %1;border-radius:6px;"
        " background:%4;color:%5;font-size:14px;white-space:nowrap;}"
        "QPushButton#pasteBtn:hover{background:%6;border-color:#3d7cf5;}"
        "QPushButton#pasteBtn:pressed{background:%7;}"
        "QLabel#mapSourceLabel{font-size:12px;}"
        "QPushButton#mapBtnFirst,QPushButton#mapBtnMid,QPushButton#mapBtnLast{"
        " padding:5px 11px;border:1px solid %1;border-radius:0;"
        " background:%4;color:%5;font-size:12px;}"
        "QPushButton#mapBtnFirst{border-top-left-radius:5px;border-bottom-left-radius:5px;border-right:none;}"
        "QPushButton#mapBtnMid{border-right:none;}"
        "QPushButton#mapBtnLast{border-top-right-radius:5px;border-bottom-right-radius:5px;}"
        "QPushButton#mapBtnFirst:checked,QPushButton#mapBtnMid:checked,QPushButton#mapBtnLast:checked{"
        " background:#3d7cf5;color:#ffffff;border-color:#2a5fd0;}"
        "QPushButton#mapBtnFirst:hover:!checked,"
        "QPushButton#mapBtnMid:hover:!checked,"
        "QPushButton#mapBtnLast:hover:!checked{background:%6;border-color:#8ab0f5;}"
    ).arg(bd, bg, fg, bb, bt, hv, pr, pb);
}

void MainWindow::applyStylesheet() {
    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
    setStyleSheet(buildStylesheet(dark));
    if (m_webBanner) {
        m_webBanner->setStyleSheet(dark
            ? "background:#1a3a1a;color:#7adc7a;padding:4px 10px;border:1px solid #2a6a2a;border-radius:4px;font-size:12px;"
            : "background:#e8f4e8;color:#1a6a1a;padding:4px 10px;border:1px solid #9acd9a;border-radius:4px;font-size:12px;");
    }
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
        applyStylesheet();
}

MainWindow::~MainWindow() {
    // Disconnect WebServer signals before QWidget destroys children.
    // When QWidget::~QWidget() runs deleteChildren(), the MainWindow vtable is
    // already replaced — assertObjectType<MainWindow> aborts on signal dispatch.
    // Disconnecting here (while MainWindow vtable is still valid) prevents it.
    if (m_webServer)
        m_webServer->disconnect(this);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Степан — твій друг в системі координат");
    setMinimumSize(620, 700);
    resize(750, 850);
    setupUi();
    applyStylesheet();

    m_webServer = new WebServer(this);
    connect(m_webServer, &WebServer::started, this, &MainWindow::onWebServerStarted);
    connect(m_webServer, &WebServer::stopped, this, &MainWindow::onWebServerStopped);
    connect(m_webServer, &WebServer::errorOccurred, this, [](const QString& msg) {
        Q_UNUSED(msg)
    });

    applyWebSettings();
}

void MainWindow::setupUi() {
    // Menu bar
    auto* settingsMenu = menuBar()->addMenu(appTr("Налаштування", "Settings"));
    auto* settingsAction = settingsMenu->addAction(appTr("Налаштування...", "Settings..."));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onShowSettings);

    auto* helpMenu = menuBar()->addMenu(appTr("Довідка", "Help"));
    auto* cliAction = helpMenu->addAction(appTr("Консольні команди...", "Command Line..."));
    connect(cliAction, &QAction::triggered, this, &MainWindow::onShowConsoleHelp);
    auto* webHelpAction = helpMenu->addAction(appTr("Веб-інтерфейс...", "Web Interface..."));
    connect(webHelpAction, &QAction::triggered, this, &MainWindow::onShowWebHelp);
    auto* bulkHelpAction = helpMenu->addAction(appTr("Масові позначки...", "Bulk Markers..."));
    connect(bulkHelpAction, &QAction::triggered, this, &MainWindow::onShowBulkMarkersHelp);
    auto* privacyAction = helpMenu->addAction(appTr("Безпека даних...", "Data Privacy..."));
    connect(privacyAction, &QAction::triggered, this, &MainWindow::onShowPrivacy);
    helpMenu->addSeparator();
    auto* aboutAction = helpMenu->addAction(appTr("Про програму...", "About Stepan..."));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    // --- Input ---
    auto* inputLbl = new QLabel(appTr("Введіть координати:", "Enter coordinates:"), central);
    inputLbl->setStyleSheet("font-weight: bold; font-size: 13px;");
    root->addWidget(inputLbl);

    m_input = new QLineEdit(central);
    m_input->setObjectName("coordInput");
    m_input->setPlaceholderText(appTr("Напр.: 37UDB1234567890  або  50.4501, 30.5234  або  50°27'00\"N 30°31'24\"E",
               "E.g.: 37UDB1234567890  or  50.4501, 30.5234  or  50°27'00\"N 30°31'24\"E"));
    m_input->setClearButtonEnabled(true);

    auto* pasteBtn = new QPushButton(appTr("Вставити", "Paste"), central);
    pasteBtn->setObjectName("pasteBtn");
    pasteBtn->setToolTip(appTr("Вставити координату з буфера обміну (Ctrl+V)", "Paste coordinate from clipboard (Ctrl+V)"));

    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(6);
    inputRow->addWidget(m_input);
    inputRow->addWidget(pasteBtn);
    root->addLayout(inputRow);

    connect(pasteBtn, &QPushButton::clicked, this, [this]() {
        const QString text = QApplication::clipboard()->text().trimmed();
        if (!text.isEmpty())
            m_input->setText(text);
        m_input->setFocus();
    });

    // --- Format detection label ---
    m_formatLbl = new QLabel(appTr("Введіть координати вище", "Enter coordinates above"), central);
    m_formatLbl->setObjectName("formatLabel");
    m_formatLbl->setStyleSheet("font-style: italic; font-size: 12px;");
    root->addWidget(m_formatLbl);

    // --- External map buttons ---
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_googleBtn = new QPushButton(appTr("Відкрити в Google Maps", "Open in Google Maps"), central);
    m_googleBtn->setObjectName("externalBtn");
    m_googleBtn->setEnabled(false);

    m_osmBtn = new QPushButton(appTr("Відкрити в OpenStreetMap", "Open in OpenStreetMap"), central);
    m_osmBtn->setObjectName("externalBtn");
    m_osmBtn->setEnabled(false);

    btnRow->addWidget(m_googleBtn);
    btnRow->addWidget(m_osmBtn);
    btnRow->addStretch();
    root->addLayout(btnRow);

    // --- Coordinates output panel ---
    m_coordsPanel = new QFrame(central);
    m_coordsPanel->setObjectName("coordsPanel");
    m_coordsPanel->setVisible(false);

    auto* coordsPanelLayout = new QVBoxLayout(m_coordsPanel);
    coordsPanelLayout->setContentsMargins(12, 8, 12, 8);
    coordsPanelLayout->setSpacing(6);

    const QString rowNames[5] = {
        "MGRS", "DD", "DMS", "UTM",
        appTr("УСК-2000", "USK-2000")
    };
    for (int i = 0; i < 5; ++i) {
        auto* rowWidget = new QWidget(m_coordsPanel);
        auto* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);

        m_rows[i].nameLbl = new QLabel(rowNames[i] + ":", rowWidget);
        m_rows[i].nameLbl->setProperty("class", "coordName");
        m_rows[i].nameLbl->setStyleSheet("font-weight: bold; font-size: 12px; min-width: 50px;");
        m_rows[i].nameLbl->setFixedWidth(55);

        m_rows[i].valueLbl = new QLabel("—", rowWidget);
        m_rows[i].valueLbl->setStyleSheet("font-family: monospace; font-size: 12px; padding: 2px 6px;");
        m_rows[i].valueLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_rows[i].valueLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        m_rows[i].copyBtn = new QPushButton(appTr("Копіювати", "Copy"), rowWidget);
        m_rows[i].copyBtn->setObjectName("copyBtn");
        m_rows[i].copyBtn->setFixedWidth(90);

        // Lambda capture by index
        const int idx = i;
        connect(m_rows[i].copyBtn, &QPushButton::clicked, this, [this, idx]() {
            if (m_rows[idx].valueLbl->text() != "—") {
                copyToClipboard(m_rows[idx].valueLbl->text());
                flashCopied(m_rows[idx].copyBtn);
            }
        });

        rowLayout->addWidget(m_rows[i].nameLbl);
        rowLayout->addWidget(m_rows[i].valueLbl);
        rowLayout->addWidget(m_rows[i].copyBtn);
        coordsPanelLayout->addWidget(rowWidget);
    }
    root->addWidget(m_coordsPanel);

    // --- Separator ---
    auto* sep2 = new QFrame(central);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep2);

    // --- Map header ---
    auto* mapHeader = new QHBoxLayout();
    mapHeader->setSpacing(8);

    auto* mapLbl = new QLabel(appTr("Карта:", "Map:"), central);
    mapLbl->setObjectName("mapSourceLabel");
    mapLbl->setStyleSheet("font-weight: bold; font-size: 12px;");

    m_mapBtnGroup = new QButtonGroup(central);
    m_mapBtnGroup->setExclusive(true);

    struct MapEntry { const char* uk; const char* en; const char* key; const char* obj; };
    const MapEntry mapEntries[] = {
        {"Вулиці (OSM)",    "Streets (OSM)",     "streets", "mapBtnFirst"},
        {"Ортофото (Esri)", "Orthoimage (Esri)", "ortho",   "mapBtnMid"},
        {"Топографічна",    "Topographic",       "topo",    "mapBtnLast"},
    };

    auto* segRow = new QHBoxLayout();
    segRow->setSpacing(0);
    segRow->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 3; ++i) {
        const auto& e = mapEntries[i];
        auto* btn = new QPushButton(appTr(e.uk, e.en), central);
        btn->setObjectName(QLatin1String(e.obj));
        btn->setCheckable(true);
        btn->setProperty("mapKey", QString::fromLatin1(e.key));
        m_mapBtnGroup->addButton(btn, i);
        segRow->addWidget(btn);
    }

    mapHeader->addWidget(mapLbl);
    mapHeader->addLayout(segRow);

    auto* bulkMarkersBtn = new QPushButton(appTr("📍 Позначки...", "📍 Markers..."), central);
    bulkMarkersBtn->setObjectName("externalBtn");
    bulkMarkersBtn->setToolTip(appTr(
        "Відкрити масовий ввід координат та позначок на карті",
        "Open bulk coordinate markers on map"));
    auto* snapshotBtn = new QPushButton(appTr("📷", "📷"), central);
    snapshotBtn->setObjectName("externalBtn");
    snapshotBtn->setToolTip(appTr(
        "Скопіювати або зберегти карту як зображення",
        "Copy or save map as image"));
    connect(snapshotBtn, &QPushButton::clicked, this, &MainWindow::onMapSnapshot);

    mapHeader->addStretch();
    mapHeader->addWidget(snapshotBtn);
    mapHeader->addWidget(bulkMarkersBtn);
    root->addLayout(mapHeader);

    // --- Map widget ---
    m_mapWidget = new MapWidget(central);
    m_mapWidget->setMinimumHeight(320);
    m_mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_mapWidget, 1);

    // --- Web server banner ---
    m_webBanner = new QLabel(central);
    m_webBanner->setOpenExternalLinks(true);
    m_webBanner->setWordWrap(true);
    m_webBanner->setVisible(false);
    root->addWidget(m_webBanner);

    // --- Connections ---
    connect(m_input,     &QLineEdit::textChanged,
            this,        &MainWindow::onInputChanged);
    connect(m_googleBtn, &QPushButton::clicked,
            this,        &MainWindow::onOpenGoogleMaps);
    connect(m_osmBtn,    &QPushButton::clicked,
            this,        &MainWindow::onOpenOSM);
    connect(m_mapBtnGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton* btn) {
        onMapSourceChanged(btn->property("mapKey").toString());
    });
    connect(bulkMarkersBtn, &QPushButton::clicked,
            this, &MainWindow::onOpenBulkMarkers);

    // Restore last-used map source
    const QString savedMap = QSettings().value("defaultMap", "streets").toString();
    for (auto* btn : m_mapBtnGroup->buttons()) {
        if (btn->property("mapKey").toString() == savedMap) {
            btn->setChecked(true);
            m_mapWidget->setLayer(savedMap);
            break;
        }
    }
    if (!m_mapBtnGroup->checkedButton())
        m_mapBtnGroup->button(0)->setChecked(true);
}

void MainWindow::onInputChanged(const QString& text) {
    m_current = CoordinateParser::parse(text);
    updateOutput(m_current);
}

void MainWindow::updateOutput(const std::optional<CoordinateData>& data) {
    const bool empty = m_input->text().trimmed().isEmpty();

    if (!data) {
        m_formatLbl->setStyleSheet(empty
            ? "color: #888; font-style: italic; font-size: 12px;"
            : "color: #c44; font-size: 12px;");
        m_formatLbl->setText(empty
            ? "Введіть координати вище"
            : appTr("Формат не розпізнано", "Format not recognized"));
        m_googleBtn->setEnabled(false);
        m_osmBtn->setEnabled(false);
        m_coordsPanel->setVisible(false);
        m_mapWidget->clearCoordinate();
        return;
    }

    // Format label — green
    m_formatLbl->setStyleSheet("color: #2a7a2a; font-weight: bold; font-size: 12px;");
    if (data->isZone()) {
        static const char* kSizes[] = {"100 км", "10 км", "1 км", "100 м", "10 м"};
        const char* sz = kSizes[data->mgrsPrecision]; // prec 0..4 is valid here
        m_formatLbl->setText(QString(appTr("MGRS — ділянка %1 × %2", "MGRS — area %1 × %2")).arg(sz).arg(sz));
    } else {
        m_formatLbl->setText(appTr("Система координат: ", "Coordinate system: ") + CoordinateParser::formatName(data->detectedFormat));
    }

    // External buttons
    m_googleBtn->setEnabled(true);
    m_osmBtn->setEnabled(true);

    // Coordinate rows
    setRow(0, data->mgrs);
    setRow(1, data->dd);
    setRow(2, data->dms);
    setRow(3, data->utm);
    setRow(4, data->usk2000);
    m_coordsPanel->setVisible(true);

    // Map
    if (data->isZone()) {
        m_mapWidget->setZone(data->zoneSWLat, data->zoneSWLon,
                             data->zoneNELat, data->zoneNELon);
    } else {
        m_mapWidget->setCoordinate(data->lat, data->lon);
    }
}

void MainWindow::setRow(int i, const QString& value) {
    m_rows[i].valueLbl->setText(value);
}

void MainWindow::onOpenGoogleMaps() {
    if (!m_current) return;
    const QString url = QString("https://www.google.com/maps/search/?api=1&query=%1,%2")
        .arg(m_current->lat, 0, 'f', 8)
        .arg(m_current->lon, 0, 'f', 8);
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::onOpenOSM() {
    if (!m_current) return;
    const QString url = QString("https://www.openstreetmap.org/?mlat=%1&mlon=%2&zoom=14")
        .arg(m_current->lat, 0, 'f', 8)
        .arg(m_current->lon, 0, 'f', 8);
    QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::onMapSourceChanged(const QString& key) {
    m_mapWidget->setLayer(key);
    QSettings().setValue("defaultMap", key);
}

void MainWindow::copyToClipboard(const QString& text) {
    QApplication::clipboard()->setText(text);
}

void MainWindow::flashCopied(QPushButton* btn) {
    const QString original = btn->text();
    btn->setText(appTr("✓ Скопійовано", "✓ Copied"));
    btn->setEnabled(false);
    QTimer::singleShot(1200, btn, [btn, original]() {
        btn->setText(original);
        btn->setEnabled(true);
    });
}

void MainWindow::setInputText(const QString& text) {
    m_input->setText(text);
    m_input->setFocus();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (QSettings().value("closeToTray", true).toBool()) {
        hide();
        event->ignore();
    } else {
        event->accept();
        QCoreApplication::quit();
    }
}

void MainWindow::onShowSettings() {
    SettingsDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    emit settingsChanged();

    // Apply map change to current session
    const QString map = QSettings().value("defaultMap", "streets").toString();
    for (auto* btn : m_mapBtnGroup->buttons()) {
        if (btn->property("mapKey").toString() == map) {
            btn->setChecked(true);
            m_mapWidget->setLayer(map);
            break;
        }
    }

    applyWebSettings();
}

void MainWindow::applyWebSettings() {
    QSettings s;
    const bool enabled = s.value("webEnabled", false).toBool();
    if (enabled) {
        const QString addr = s.value("webAddress", "0.0.0.0").toString();
        const quint16 port = static_cast<quint16>(s.value("webPort", 8080).toInt());
        if (!m_webServer->isRunning())
            m_webServer->start(addr, port);
    } else {
        if (m_webServer->isRunning())
            m_webServer->stop();
    }
}

void MainWindow::onWebServerStarted(quint16 port) {
    QSettings s;
    const QString addr = s.value("webAddress", "0.0.0.0").toString();
    const QStringList urls = WebServer::listenUrls(addr, port);
    QStringList links;
    for (const QString& url : urls)
        links << QString("<a href='%1' style='color:#1a6a1a;'>%1</a>").arg(url);
    m_webBanner->setText(appTr("Веб-сервер: ", "Web server: ") + links.join("&nbsp;&nbsp;·&nbsp;&nbsp;"));
    m_webBanner->setVisible(true);
}

void MainWindow::onWebServerStopped() {
    m_webBanner->setVisible(false);
}

void MainWindow::onShowConsoleHelp() {
    static const QString kHtmlUk = R"(
<style>
  body  { font-family: sans-serif; font-size: 13px; margin: 0; }
  h2    { margin-bottom: 4px; }
  h3    { margin: 12px 0 4px; color: #2a5a9a; }
  pre   { background: #f4f4f4; border-radius: 4px; padding: 8px 12px;
          font-size: 12px; white-space: pre-wrap; line-height: 1.5; }
  code  { background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  table { border-collapse: collapse; width: 100%; }
  td    { padding: 3px 8px; vertical-align: top; }
  td:first-child { white-space: nowrap; font-family: monospace; font-size: 12px; color: #333; }
  .comment { color: #6a9955; }
</style>
<h2>Консольні команди</h2>
<p>Програму можна запускати з командного рядка без відкриття вікна.</p>

<h3>Використання</h3>
<pre>stepan [ОПЦІЇ] [КООРДИНАТА]</pre>

<h3>Опції</h3>
<table>
  <tr><td>-c, --convert &lt;координата&gt;</td>
      <td>Конвертувати координату та вивести результат</td></tr>
  <tr><td>-f, --format &lt;формат&gt;</td>
      <td>Вивести лише вказаний формат:
          <code>mgrs</code> | <code>dd</code> | <code>dms</code> | <code>utm</code> | <code>usk2000</code></td></tr>
  <tr><td>-r, --raw</td>
      <td>Вивести значення без форматування — без пробілів, дефісів та символів °N/°E</td></tr>
  <tr><td>-u, --url &lt;сервіс&gt;</td>
      <td>Генерувати посилання на карту:
          <code>google</code> | <code>osm</code> | <code>bing</code> | <code>all</code></td></tr>
  <tr><td>-o, --open &lt;координата&gt;</td>
      <td>Відкрити GUI з попередньо заповненою координатою</td></tr>
  <tr><td>-v, --version</td>
      <td>Показати версію програми та вийти</td></tr>
  <tr><td>-h, --help</td>
      <td>Показати довідку та вийти</td></tr>
</table>

<h3>Позиційний аргумент</h3>
<table>
  <tr><td>КООРДИНАТА</td>
      <td>Відкрити GUI з цією координатою (еквівалент <code>--open</code>)</td></tr>
</table>

<h3>Приклади</h3>
<pre><span class="comment"># Конвертувати з DD — вивести всі формати</span>
$ stepan --convert "48.020991, 37.810247"
MGRS:     37U DP 11286 19318
DD:       48.020991°N, 37.810247°E
DMS:      48°01'15.57"N 37°48'36.89"E
UTM:      37N 411286 5319318
УСК-2000: 53-21447 74-11251

<span class="comment"># Конвертувати з MGRS — вивести всі формати</span>
$ stepan --convert "37U DP 11286 19318"
MGRS:     37U DP 11286 19318
DD:       48.020995°N, 37.810248°E
DMS:      48°01'15.58"N 37°48'36.89"E
UTM:      37N 411286 5319319
УСК-2000: 53-21447 74-11251

<span class="comment"># Вивести лише MGRS</span>
$ stepan -c "48.020991, 37.810247" -f mgrs
37U DP 11286 19318

<span class="comment"># Вивести лише УСК-2000</span>
$ stepan -c "37U DP 11286 19318" -f usk2000
53-21447 74-11251

<span class="comment"># Неформатований вивід — всі формати</span>
$ stepan -c "48.020991, 37.810247" --raw
MGRS:     37UDP1128619318
DD:       48.020991,37.810247
DMS:      48°01'15.57"N37°48'36.89"E
UTM:      37N,411286,5319318
УСК-2000: 5321447 7411251

<span class="comment"># Неформатований MGRS</span>
$ stepan -c "48.020991, 37.810247" -f mgrs --raw
37UDP1128619318

<span class="comment"># Посилання на Google Maps</span>
$ stepan -c "48.020991, 37.810247" --url google
https://www.google.com/maps/search/?api=1&query=48.020991,37.810247

<span class="comment"># Посилання на всі підтримувані сервіси</span>
$ stepan -c "37U DP 11286 19318" --url all
Google Maps    https://www.google.com/maps/search/?api=1&query=48.020995,37.810248
OpenStreetMap  https://www.openstreetmap.org/?mlat=48.020995&mlon=37.810248&zoom=14
Bing Maps      https://www.bing.com/maps/?cp=48.020995~37.810248&lvl=14

<span class="comment"># MGRS + посилання на OSM</span>
$ stepan -c "48.020991, 37.810247" -f mgrs --url osm
37U DP 11286 19318
https://www.openstreetmap.org/?mlat=48.020991&mlon=37.810247&zoom=14

<span class="comment"># Відкрити GUI з координатою</span>
$ stepan --open "37U DP 11286 19318"

<span class="comment"># Відкрити GUI (скорочений запис)</span>
$ stepan "48.020991, 37.810247"</pre>

<h3>Підтримувані формати вводу</h3>
<table>
  <tr><td>MGRS</td>    <td><code>37UDP1128619318</code> або <code>37U DP 11286 19318</code></td></tr>
  <tr><td>DD</td>      <td><code>48.020991, 37.810247</code></td></tr>
  <tr><td>DMS</td>     <td><code>48°01'15.57"N 37°48'36.89"E</code></td></tr>
  <tr><td>UTM</td>     <td><code>37N 411286 5319318</code></td></tr>
  <tr><td>УСК-2000</td><td><code>53-21447 74-11251</code></td></tr>
</table>

<h3>Примітка</h3>
<p>На macOS запускайте бінарний файл напряму:<br>
<code>./Stepan.app/Contents/MacOS/Stepan --convert "..."</code></p>
)";

    static const QString kHtmlEn = R"(
<style>
  body  { font-family: sans-serif; font-size: 13px; margin: 0; }
  h2    { margin-bottom: 4px; }
  h3    { margin: 12px 0 4px; color: #2a5a9a; }
  pre   { background: #f4f4f4; border-radius: 4px; padding: 8px 12px;
          font-size: 12px; white-space: pre-wrap; line-height: 1.5; }
  code  { background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  table { border-collapse: collapse; width: 100%; }
  td    { padding: 3px 8px; vertical-align: top; }
  td:first-child { white-space: nowrap; font-family: monospace; font-size: 12px; color: #333; }
  .comment { color: #6a9955; }
</style>
<h2>Command Line</h2>
<p>The application can be launched from the command line without opening a window.</p>

<h3>Usage</h3>
<pre>stepan [OPTIONS] [COORDINATE]</pre>

<h3>Options</h3>
<table>
  <tr><td>-c, --convert &lt;coordinate&gt;</td>
      <td>Convert coordinate and output result</td></tr>
  <tr><td>-f, --format &lt;format&gt;</td>
      <td>Output only the specified format:
          <code>mgrs</code> | <code>dd</code> | <code>dms</code> | <code>utm</code> | <code>usk2000</code></td></tr>
  <tr><td>-r, --raw</td>
      <td>Output unformatted values — no spaces, dashes or °N/°E symbols</td></tr>
  <tr><td>-u, --url &lt;service&gt;</td>
      <td>Generate map link:
          <code>google</code> | <code>osm</code> | <code>bing</code> | <code>all</code></td></tr>
  <tr><td>-o, --open &lt;coordinate&gt;</td>
      <td>Open GUI with pre-filled coordinate</td></tr>
  <tr><td>-v, --version</td>
      <td>Show application version and exit</td></tr>
  <tr><td>-h, --help</td>
      <td>Show help and exit</td></tr>
</table>

<h3>Positional argument</h3>
<table>
  <tr><td>COORDINATE</td>
      <td>Open GUI with this coordinate (equivalent to <code>--open</code>)</td></tr>
</table>

<h3>Examples</h3>
<pre><span class="comment"># Convert from DD — output all formats</span>
$ stepan --convert "48.020991, 37.810247"
MGRS:    37U DP 11286 19318
DD:      48.020991°N, 37.810247°E
DMS:     48°01'15.57"N 37°48'36.89"E
UTM:     37N 411286 5319318
USK-2000: 53-21447 74-11251

<span class="comment"># Convert from MGRS — output all formats</span>
$ stepan --convert "37U DP 11286 19318"
MGRS:    37U DP 11286 19318
DD:      48.020995°N, 37.810248°E
DMS:     48°01'15.58"N 37°48'36.89"E
UTM:     37N 411286 5319319
USK-2000: 53-21447 74-11251

<span class="comment"># Output MGRS only</span>
$ stepan -c "48.020991, 37.810247" -f mgrs
37U DP 11286 19318

<span class="comment"># Output USK-2000 only</span>
$ stepan -c "37U DP 11286 19318" -f usk2000
53-21447 74-11251

<span class="comment"># Unformatted output — all formats</span>
$ stepan -c "48.020991, 37.810247" --raw
MGRS:    37UDP1128619318
DD:      48.020991,37.810247
DMS:     48°01'15.57"N37°48'36.89"E
UTM:     37N,411286,5319318
USK-2000: 5321447 7411251

<span class="comment"># Unformatted MGRS</span>
$ stepan -c "48.020991, 37.810247" -f mgrs --raw
37UDP1128619318

<span class="comment"># Google Maps link</span>
$ stepan -c "48.020991, 37.810247" --url google
https://www.google.com/maps/search/?api=1&query=48.020991,37.810247

<span class="comment"># Links for all supported services</span>
$ stepan -c "37U DP 11286 19318" --url all
Google Maps    https://www.google.com/maps/search/?api=1&query=48.020995,37.810248
OpenStreetMap  https://www.openstreetmap.org/?mlat=48.020995&mlon=37.810248&zoom=14
Bing Maps      https://www.bing.com/maps/?cp=48.020995~37.810248&lvl=14

<span class="comment"># MGRS + OSM link</span>
$ stepan -c "48.020991, 37.810247" -f mgrs --url osm
37U DP 11286 19318
https://www.openstreetmap.org/?mlat=48.020991&mlon=37.810247&zoom=14

<span class="comment"># Open GUI with coordinate</span>
$ stepan --open "37U DP 11286 19318"

<span class="comment"># Open GUI (short form)</span>
$ stepan "48.020991, 37.810247"</pre>

<h3>Supported input formats</h3>
<table>
  <tr><td>MGRS</td>   <td><code>37UDP1128619318</code> or <code>37U DP 11286 19318</code></td></tr>
  <tr><td>DD</td>     <td><code>48.020991, 37.810247</code></td></tr>
  <tr><td>DMS</td>    <td><code>48°01'15.57"N 37°48'36.89"E</code></td></tr>
  <tr><td>UTM</td>    <td><code>37N 411286 5319318</code></td></tr>
  <tr><td>USK-2000</td><td><code>53-21447 74-11251</code></td></tr>
</table>

<h3>Note</h3>
<p>On macOS, run the binary directly:<br>
<code>./Stepan.app/Contents/MacOS/Stepan --convert "..."</code></p>
)";

    const QString& kHtml = appLangIsEnglish() ? kHtmlEn : kHtmlUk;

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(appTr("Консольні команди", "Command Line"));
    dlg->resize(600, 540);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto* browser = new QTextBrowser(dlg);
    browser->setHtml(kHtml);
    browser->setOpenExternalLinks(false);
    browser->setReadOnly(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::accept);

    auto* layout = new QVBoxLayout(dlg);
    layout->addWidget(browser);
    layout->addWidget(buttons);

    dlg->exec();
}

void MainWindow::onShowWebHelp() {
    static const QString kHtmlUk = R"(
<style>
  body  { font-family: sans-serif; font-size: 13px; margin: 0; }
  h2    { margin-bottom: 4px; }
  h3    { margin: 12px 0 4px; color: #2a5a9a; }
  pre   { background: #f4f4f4; border-radius: 4px; padding: 8px 12px;
          font-size: 12px; white-space: pre-wrap; line-height: 1.5; }
  code  { background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  table { border-collapse: collapse; width: 100%; }
  td    { padding: 3px 8px; vertical-align: top; }
  td:first-child { white-space: nowrap; font-family: monospace; font-size: 12px; color: #333; }
  .comment { color: #6a9955; }
</style>
<h2>Веб-інтерфейс</h2>
<p>Вбудований веб-сервер дозволяє конвертувати координати з будь-якого пристрою в мережі через браузер.</p>

<h3>Увімкнення</h3>
<p>Відкрийте <b>Налаштування → Веб-інтерфейс</b>, поставте прапорець
<i>Увімкнути вбудований веб-сервер</i>, оберіть мережевий інтерфейс і порт.
URL для доступу відображається там же та в нижньому банері головного вікна після запуску.</p>
<table>
  <tr><td>Всі мережі (0.0.0.0)</td>
      <td>Доступно з інших пристроїв у локальній мережі та локально</td></tr>
  <tr><td>Лише локальний (127.0.0.1)</td>
      <td>Доступно лише на цьому комп'ютері</td></tr>
</table>

<h3>Браузерний інтерфейс</h3>
<p>Відкрийте URL у будь-якому браузері — наприклад, <code>http://192.168.1.5:8080/</code>.
Введіть координату у поле та натисніть <i>Конвертувати</i>. Результат відображається
у всіх підтримуваних форматах із кнопками копіювання та посиланнями на карти.</p>

<h3>HTTP API</h3>
<p>Для програмного використання підтримується GET-запит:</p>
<pre>GET /api/convert?q=&lt;координата&gt;</pre>
<p>Координату потрібно передавати у URL-кодуванні. Відповідь — JSON:</p>
<pre><span class="comment"># Приклад запиту</span>
$ curl "http://localhost:8080/api/convert?q=48.020991%2C37.810247"
{
  "success": true,
  "format":  "DD",
  "mgrs":    "37U DP 11286 19318",
  "dd":      "48.020991°N, 37.810247°E",
  "dms":     "48°01'15.57\"N 37°48'36.89\"E",
  "utm":     "37N 411286 5319318",
  "usk2000": "53-21447 74-11251",
  "lat":     48.020991,
  "lon":     37.810247
}

<span class="comment"># Приклад з MGRS</span>
$ curl "http://localhost:8080/api/convert?q=37U+DP+11286+19318"
{"success":true,"format":"MGRS",...}

<span class="comment"># При помилці</span>
$ curl "http://localhost:8080/api/convert?q=invalid"
{"success":false,"error":"Формат не розпізнано"}</pre>
)";

    static const QString kHtmlEn = R"(
<style>
  body  { font-family: sans-serif; font-size: 13px; margin: 0; }
  h2    { margin-bottom: 4px; }
  h3    { margin: 12px 0 4px; color: #2a5a9a; }
  pre   { background: #f4f4f4; border-radius: 4px; padding: 8px 12px;
          font-size: 12px; white-space: pre-wrap; line-height: 1.5; }
  code  { background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  table { border-collapse: collapse; width: 100%; }
  td    { padding: 3px 8px; vertical-align: top; }
  td:first-child { white-space: nowrap; font-family: monospace; font-size: 12px; color: #333; }
  .comment { color: #6a9955; }
</style>
<h2>Web Interface</h2>
<p>The built-in web server allows converting coordinates from any device on the network through a browser.</p>

<h3>Enabling</h3>
<p>Open <b>Settings → Web Interface</b>, check <i>Enable built-in web server</i>, choose the network interface and port.
The access URL is shown there and in the banner at the bottom of the main window after startup.</p>
<table>
  <tr><td>All networks (0.0.0.0)</td>
      <td>Accessible from other devices on the local network and locally</td></tr>
  <tr><td>Local only (127.0.0.1)</td>
      <td>Accessible only on this computer</td></tr>
</table>

<h3>Browser interface</h3>
<p>Open the URL in any browser — e.g. <code>http://192.168.1.5:8080/</code>.
Enter a coordinate and click <i>Convert</i>. The result is shown in all supported formats
with copy buttons and map links.</p>

<h3>HTTP API</h3>
<p>For programmatic use, a GET request is supported:</p>
<pre>GET /api/convert?q=&lt;coordinate&gt;</pre>
<p>The coordinate must be URL-encoded. Response is JSON:</p>
<pre><span class="comment"># Example request</span>
$ curl "http://localhost:8080/api/convert?q=48.020991%2C37.810247"
{
  "success": true,
  "format":  "DD",
  "mgrs":    "37U DP 11286 19318",
  "dd":      "48.020991°N, 37.810247°E",
  "dms":     "48°01'15.57\"N 37°48'36.89\"E",
  "utm":     "37N 411286 5319318",
  "usk2000": "53-21447 74-11251",
  "lat":     48.020991,
  "lon":     37.810247
}

<span class="comment"># Example with MGRS</span>
$ curl "http://localhost:8080/api/convert?q=37U+DP+11286+19318"
{"success":true,"format":"MGRS",...}

<span class="comment"># On error</span>
$ curl "http://localhost:8080/api/convert?q=invalid"
{"success":false,"error":"Format not recognized"}</pre>
)";

    const QString& kHtml = appLangIsEnglish() ? kHtmlEn : kHtmlUk;

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(appTr("Веб-інтерфейс", "Web Interface"));
    dlg->resize(560, 460);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto* browser = new QTextBrowser(dlg);
    browser->setHtml(kHtml);
    browser->setOpenExternalLinks(false);
    browser->setReadOnly(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::accept);

    auto* layout = new QVBoxLayout(dlg);
    layout->addWidget(browser);
    layout->addWidget(buttons);

    dlg->exec();
}

// QTextBrowser subclass that loads images from QRC via the "qrc" scheme
// Fullscreen overlay shown when the user clicks a help image
class ImageLightbox : public QWidget {
    QPixmap m_px;
public:
    ImageLightbox(const QImage& img, QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_DeleteOnClose);
        setGeometry(parent->rect());

        QSize maxSz = parent->size() - QSize(60, 60);
        m_px = QPixmap::fromImage(img);
        if (m_px.width() > maxSz.width() || m_px.height() > maxSz.height())
            m_px = m_px.scaled(maxSz, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        show();
        raise();
        setFocus();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(0, 0, 0, 205));
        p.drawPixmap((width() - m_px.width()) / 2,
                     (height() - m_px.height()) / 2, m_px);
        p.setPen(QColor(255, 255, 255, 130));
        p.setFont(QFont("sans-serif", 10));
        p.drawText(rect().adjusted(0, 0, -10, -8),
                   Qt::AlignRight | Qt::AlignBottom,
                   appTr("Натисніть для закриття  ·  Esc",
                         "Click to close  ·  Esc"));
    }

    void mousePressEvent(QMouseEvent*) override { close(); }

    void keyPressEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Escape) close();
        else QWidget::keyPressEvent(e);
    }
};

class BulkHelpBrowser : public QTextBrowser {
public:
    explicit BulkHelpBrowser(QWidget* parent = nullptr) : QTextBrowser(parent) {
        viewport()->setMouseTracking(true);
    }

    QVariant loadResource(int type, const QUrl& url) override {
        if (type == QTextDocument::ImageResource && url.scheme() == "qrc") {
            QImage img(":" + url.path());
            if (!img.isNull()) {
                if (img.width() > 640)
                    img = img.scaledToWidth(640, Qt::SmoothTransformation);
                return img;
            }
        }
        return QTextBrowser::loadResource(type, url);
    }

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            auto fmt = charFormatAt(e->pos());
            if (fmt.isImageFormat()) {
                QImage img(":" + QUrl(fmt.toImageFormat().name()).path());
                if (!img.isNull()) {
                    new ImageLightbox(img, window());
                    return;
                }
            }
        }
        QTextBrowser::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        QTextBrowser::mouseMoveEvent(e);
        if (charFormatAt(e->pos()).isImageFormat())
            viewport()->setCursor(Qt::PointingHandCursor);
    }

private:
    QTextCharFormat charFormatAt(const QPoint& pos) {
        QTextCursor c = cursorForPosition(pos);
        QTextCharFormat fmt = c.charFormat();
        if (!fmt.isImageFormat()) {
            c.movePosition(QTextCursor::PreviousCharacter);
            fmt = c.charFormat();
        }
        return fmt;
    }
};

void MainWindow::onShowBulkMarkersHelp() {
    static const QString kHtmlUk = R"(
<style>
  body  { font-family: sans-serif; font-size: 13px; margin: 0; line-height: 1.6; }
  h2    { margin-bottom: 6px; }
  h3    { margin: 14px 0 4px; color: #2a5a9a; }
  p     { margin: 4px 0 8px; }
  ul    { margin: 4px 0 8px; padding-left: 20px; }
  li    { margin-bottom: 3px; }
  code  { background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  .img-wrap { margin: 12px 0; border: 1px solid #ddd; border-radius: 4px; overflow: hidden; }
  .img-wrap img { max-width: 100%; display: block; }
  table { border-collapse: collapse; width: 100%; margin: 4px 0 8px; }
  td    { padding: 3px 8px; vertical-align: top; }
  td:first-child { white-space: nowrap; font-weight: bold; font-size: 12px; color: #444; }
</style>
<h2>Масові позначки на карті</h2>
<p>Функція дозволяє завантажити список точок у будь-якому форматі координат,
відобразити їх на карті та зберегти у вигляді HTML-файлу з інтерактивною картою.</p>

<h3>Як відкрити</h3>
<p>Кнопка <b>Позначки</b> у нижній частині головного вікна програми.</p>

<div class="img-wrap"><img src="qrc:///doc/PointList.png"></div>

<h3>Введення координат</h3>
<p>Кожен рядок — одна точка. Рядки з помилками підсвічуються
<span style="color:#c00; font-weight:bold">червоним кольором</span>.
Кількість коректних та некоректних рядків відображається під полем введення.</p>

<h3>Порядок: «Спочатку координати» / «Спочатку опис»</h3>
<p>Визначає, де у рядку знаходиться підпис — після координат (стандартний режим)
або перед ними (зручно для списків виду «Назва об'єкта — координати»).</p>

<h3>Формат рядка</h3>
<table>
  <tr><td>MGRS - Підпис</td>         <td><code>37U CP 94712 95805 - Точка А</code></td></tr>
  <tr><td>DD Підпис</td>             <td><code>48.70639, 37.56889 Точка Б</code></td></tr>
  <tr><td>DMS - Підпис</td>          <td><code>48°01'15"N 37°48'36"E - Позиція</code></td></tr>
  <tr><td>UTM Підпис</td>            <td><code>37N 411286 5319318 КП</code></td></tr>
  <tr><td>УСК-2000 Підпис</td>       <td><code>53-21447 74-11251 Ціль</code></td></tr>
  <tr><td>Підпис - MGRS</td>         <td><code>Нафтобаза - 37U CP 94712 95805</code> (режим «Спочатку опис»)</td></tr>
</table>
<p>Роздільник між координатами та підписом: <code>&nbsp;-&nbsp;</code>
або <code>:&nbsp;</code> або пробіл. Підпис є необов'язковим.</p>

<h3>Кнопки</h3>
<table>
  <tr><td>Показати на карті</td>
      <td>Відображає всі точки у головному вікні та автоматично масштабує карту.</td></tr>
  <tr><td>Завантажити HTML</td>
      <td>Зберігає HTML-файл з інтерактивною картою (Leaflet, OpenStreetMap). Відкрийте у браузері — інтернет потрібен лише для завантаження тайлів карти.</td></tr>
  <tr><td>Очистити карту</td>
      <td>Прибирає всі позначки з карти у головному вікні програми.</td></tr>
</table>

<h3>Таблиця MGRS-координат у HTML-файлі</h3>
<p>Якщо увімкнено опцію <b>«Показати таблицю MGRS-координат у завантаженому HTML-файлі»</b>,
у збереженому файлі з'явиться таблиця у правому нижньому куті зі списком усіх точок.</p>

<div class="img-wrap"><img src="qrc:///doc/PointListMap.png"></div>

<ul>
  <li>Таблиця напівпрозора за замовчуванням — наведіть курсор, щоб побачити її повністю.</li>
  <li>Клік на рядок таблиці — карта центрується на точці, відкривається спливаюче вікно з координатами та MGRS.</li>
  <li>Клік на будь-яку позначку — відображається підпис, MGRS-координата та десяткові координати.</li>
</ul>
)";

    static const QString kHtmlEn = R"(
<style>
  body  { font-family: sans-serif; font-size: 13px; margin: 0; line-height: 1.6; }
  h2    { margin-bottom: 6px; }
  h3    { margin: 14px 0 4px; color: #2a5a9a; }
  p     { margin: 4px 0 8px; }
  ul    { margin: 4px 0 8px; padding-left: 20px; }
  li    { margin-bottom: 3px; }
  code  { background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  .img-wrap { margin: 12px 0; border: 1px solid #ddd; border-radius: 4px; overflow: hidden; }
  .img-wrap img { max-width: 100%; display: block; }
  table { border-collapse: collapse; width: 100%; margin: 4px 0 8px; }
  td    { padding: 3px 8px; vertical-align: top; }
  td:first-child { white-space: nowrap; font-weight: bold; font-size: 12px; color: #444; }
</style>
<h2>Bulk Markers</h2>
<p>This feature lets you load a list of coordinates in any supported format,
display them on the map, and save as a standalone HTML file with an interactive map.</p>

<h3>How to open</h3>
<p>Click the <b>Markers</b> button at the bottom of the main window.</p>

<div class="img-wrap"><img src="qrc:///doc/PointList.png"></div>

<h3>Entering coordinates</h3>
<p>One point per line. Lines with errors are highlighted
<span style="color:#c00; font-weight:bold">in red</span>.
The count of valid and invalid lines is shown below the input field.</p>

<h3>Order: «Coords first» / «Label first»</h3>
<p>Choose whether the label comes after the coordinates (default) or before them
(convenient for lists like «Object name — coordinates»).</p>

<h3>Line format</h3>
<table>
  <tr><td>MGRS - Label</td>          <td><code>37U CP 94712 95805 - Point A</code></td></tr>
  <tr><td>DD Label</td>              <td><code>48.70639, 37.56889 Point B</code></td></tr>
  <tr><td>DMS - Label</td>           <td><code>48°01'15"N 37°48'36"E - Position</code></td></tr>
  <tr><td>UTM Label</td>             <td><code>37N 411286 5319318 CP</code></td></tr>
  <tr><td>USK-2000 Label</td>        <td><code>53-21447 74-11251 Target</code></td></tr>
  <tr><td>Label - MGRS</td>          <td><code>Refinery - 37U CP 94712 95805</code> (Label first mode)</td></tr>
</table>
<p>Separator between coordinates and label: <code>&nbsp;-&nbsp;</code>
or <code>:&nbsp;</code> or space. Label is optional.</p>

<h3>Buttons</h3>
<table>
  <tr><td>Show on map</td>
      <td>Displays all valid points in the main window and auto-fits the map view.</td></tr>
  <tr><td>Download HTML</td>
      <td>Saves an HTML file with an interactive map (Leaflet, OpenStreetMap). Internet is only needed for loading map tiles.</td></tr>
  <tr><td>Clear map</td>
      <td>Removes all bulk markers from the map in the main window.</td></tr>
</table>

<h3>MGRS table in the HTML file</h3>
<p>When <b>«Show MGRS coordinate table in the downloaded HTML file»</b> is enabled,
the saved file will include a table panel in the bottom-right corner listing all points.</p>

<div class="img-wrap"><img src="qrc:///doc/PointListMap.png"></div>

<ul>
  <li>The table is semi-transparent by default — hover over it to see it fully.</li>
  <li>Click a row — the map centers on that point and a popup opens with coordinates and MGRS.</li>
  <li>Click any marker — shows label, MGRS coordinate, and decimal coordinates.</li>
</ul>
)";

    const QString& kHtml = appLangIsEnglish() ? kHtmlEn : kHtmlUk;

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(appTr("Масові позначки", "Bulk Markers"));
    dlg->resize(680, 600);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto* browser = new BulkHelpBrowser(dlg);
    browser->setReadOnly(true);
    browser->setOpenExternalLinks(false);
    browser->setHtml(kHtml);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::accept);

    auto* layout = new QVBoxLayout(dlg);
    layout->addWidget(browser);
    layout->addWidget(buttons);

    dlg->exec();
}

void MainWindow::onShowPrivacy() {
    static const QString kHtmlUk = R"prv(
<style>
  body { font-family: sans-serif; font-size: 13px; margin: 0; }
  h2   { margin-bottom: 6px; }
  h3   { margin: 14px 0 4px; color: #2a5a9a; }
  ul   { margin: 4px 0 4px 0; padding-left: 20px; }
  li   { margin-bottom: 3px; }
  .ok  { color: #2a7a2a; font-weight: bold; }
  .warn{ color: #7a5a00; font-weight: bold; }
  .key { font-family: monospace; background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  .box { background: #f0f6e8; border: 1px solid #b8d4a0; border-radius: 5px; padding: 8px 12px; margin-top: 10px; }
  .mil { background: #fff8e8; border: 1px solid #e0b840; border-radius: 5px; padding: 10px 14px; margin-bottom: 12px; }
</style>
<h2>Безпека даних</h2>

<div class="mil">
<b>Цей застосунок розроблений насамперед для військових.</b><br>
Координати — це оперативна інформація. Тому безпека даних тут не формальність,
а принципова вимога до архітектури програми:<br><br>
<span class="ok">✓</span> &nbsp;Жодна введена координата не залишає пристрій без вашого явного дозволу.<br>
<span class="ok">✓</span> &nbsp;Програма не має власних серверів і не телефонує додому.<br>
<span class="ok">✓</span> &nbsp;Веб-сервер вимкнений за замовчуванням; якщо увімкнено — лише локальна мережа.<br>
<span class="warn">⚠</span> &nbsp;Карта завантажує тайли з зовнішніх серверів — деталі нижче.
</div>

<p>Нижче — повний опис того, що відбувається з вашими даними.</p>

<h3>Що зберігається локально</h3>
<p>Лише налаштування програми, через стандартний механізм ОС (<span class="key">QSettings</span>):</p>
<ul>
  <li>Джерело карти за замовчуванням (<span class="key">defaultMap</span>)</li>
  <li>Стиль іконки трею (<span class="key">trayIconStyle</span>)</li>
  <li>Поведінка при закритті вікна (<span class="key">closeToTray</span>)</li>
  <li>Мова інтерфейсу (<span class="key">language</span>)</li>
  <li>Запускати мінімізованим (<span class="key">startMinimized</span>)</li>
  <li>Параметри веб-сервера: увімкнено / адреса / порт (<span class="key">webEnabled</span>, <span class="key">webAddress</span>, <span class="key">webPort</span>)</li>
  <li>Автозапуск при вході в систему — через засоби ОС</li>
</ul>
<p><span class="ok">✓</span> &nbsp;Введені координати <b>не зберігаються</b> — ні в файлах, ні в налаштуваннях.</p>

<h3>Що не збирається ніколи</h3>
<ul>
  <li>Телеметрія та аналітика</li>
  <li>Журнали використання</li>
  <li>Геолокація пристрою</li>
  <li>Будь-які персональні дані</li>
</ul>

<h3>Зовнішні сервіси (карти)</h3>
<p>При роботі з картою координати або область перегляду можуть передаватись стороннім серверам:</p>
<ul>
  <li><span class="warn">⚠</span> &nbsp;<b>Тайли карти</b> — завантажуються з серверів OpenStreetMap, Esri або інших
      залежно від вибраного джерела. Ці запити містять координати видимої ділянки карти.</li>
  <li><span class="warn">⚠</span> &nbsp;<b>«Відкрити в Google Maps / OpenStreetMap»</b> — відкриває у вашому браузері URL
      з поточними координатами. Дані обробляються відповідно до політики конфіденційності Google та OpenStreetMap.</li>
</ul>

<h3>Веб-сервер (якщо увімкнено)</h3>
<ul>
  <li>Веб-сервер слухає виключно на вашому пристрої або у локальній мережі — він <b>не передає дані в інтернет</b>.</li>
  <li>Якщо обрано режим <b>«Всі мережі (0.0.0.0)»</b>, координати, що надсилаються через HTTP-запити,
      будуть доступні іншим пристроям у вашій локальній мережі.</li>
  <li>Для максимальної приватності використовуйте режим <b>«Лише локальний (127.0.0.1)»</b>.</li>
</ul>

<div class="box">
  <span class="ok">✓</span> &nbsp;Програма не має власних серверів і не відправляє дані розробнику.
  Весь код відкритий і доступний для перевірки.
</div>
)prv";

    static const QString kHtmlEn = R"prv(
<style>
  body { font-family: sans-serif; font-size: 13px; margin: 0; }
  h2   { margin-bottom: 6px; }
  h3   { margin: 14px 0 4px; color: #2a5a9a; }
  ul   { margin: 4px 0 4px 0; padding-left: 20px; }
  li   { margin-bottom: 3px; }
  .ok  { color: #2a7a2a; font-weight: bold; }
  .warn{ color: #7a5a00; font-weight: bold; }
  .key { font-family: monospace; background: #eee; padding: 1px 4px; border-radius: 3px; font-size: 12px; }
  .box { background: #f0f6e8; border: 1px solid #b8d4a0; border-radius: 5px; padding: 8px 12px; margin-top: 10px; }
  .mil { background: #fff8e8; border: 1px solid #e0b840; border-radius: 5px; padding: 10px 14px; margin-bottom: 12px; }
</style>
<h2>Data Privacy</h2>

<div class="mil">
<b>This app is built primarily for military use.</b><br>
Coordinates are operational information. Data security here is not a formality —
it is a core architectural requirement:<br><br>
<span class="ok">✓</span> &nbsp;No entered coordinate leaves the device without your explicit action.<br>
<span class="ok">✓</span> &nbsp;The app has no backend servers and never phones home.<br>
<span class="ok">✓</span> &nbsp;The web server is off by default; when enabled — local network only.<br>
<span class="warn">⚠</span> &nbsp;The map loads tiles from external servers — see details below.
</div>

<p>Below is a full account of what happens to your data.</p>

<h3>What is stored locally</h3>
<p>Only application preferences, via the OS standard mechanism (<span class="key">QSettings</span>):</p>
<ul>
  <li>Default map source (<span class="key">defaultMap</span>)</li>
  <li>Tray icon style (<span class="key">trayIconStyle</span>)</li>
  <li>Close-to-tray behaviour (<span class="key">closeToTray</span>)</li>
  <li>Interface language (<span class="key">language</span>)</li>
  <li>Start minimized flag (<span class="key">startMinimized</span>)</li>
  <li>Web server settings: enabled / address / port (<span class="key">webEnabled</span>, <span class="key">webAddress</span>, <span class="key">webPort</span>)</li>
  <li>Launch at login — managed via OS facilities</li>
</ul>
<p><span class="ok">✓</span> &nbsp;Entered coordinates are <b>never stored</b> — not in files, not in settings.</p>

<h3>What is never collected</h3>
<ul>
  <li>Telemetry or analytics</li>
  <li>Usage logs</li>
  <li>Device location</li>
  <li>Any personal data</li>
</ul>

<h3>Third-party services (maps)</h3>
<p>When using the map, coordinates or the viewed area may be sent to third-party servers:</p>
<ul>
  <li><span class="warn">⚠</span> &nbsp;<b>Map tiles</b> — downloaded from OpenStreetMap, Esri or other servers
      depending on the selected source. These requests include the coordinates of the visible map area.</li>
  <li><span class="warn">⚠</span> &nbsp;<b>"Open in Google Maps / OpenStreetMap"</b> — opens a URL in your browser
      containing the current coordinates. Data is handled according to Google's and OpenStreetMap's privacy policies.</li>
</ul>

<h3>Web server (if enabled)</h3>
<ul>
  <li>The web server listens only on your device or local network — it <b>does not send data to the internet</b>.</li>
  <li>If <b>"All networks (0.0.0.0)"</b> is selected, coordinates sent via HTTP requests will be
      visible to other devices on your local network.</li>
  <li>For maximum privacy, use <b>"Local only (127.0.0.1)"</b>.</li>
</ul>

<div class="box">
  <span class="ok">✓</span> &nbsp;The app has no backend servers and sends no data to the developer.
  The full source code is open and available for review.
</div>
)prv";

    const QString& kHtml = appLangIsEnglish() ? kHtmlEn : kHtmlUk;

    auto* dlg = new QDialog(this);
    dlg->setWindowTitle(appTr("Безпека даних", "Data Privacy"));
    dlg->resize(560, 560);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto* browser = new QTextBrowser(dlg);
    browser->setHtml(kHtml);
    browser->setReadOnly(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
    connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::accept);

    auto* layout = new QVBoxLayout(dlg);
    layout->addWidget(browser);
    layout->addWidget(buttons);

    dlg->exec();
}

void MainWindow::onShowAbout() {
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onOpenBulkMarkers() {
    auto* dlg = new BulkMarkersDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &BulkMarkersDialog::markersReady,
            this, [this](const QVector<BulkMarker>& markers) {
        m_mapWidget->setBulkMarkers(markers);
    });
    connect(dlg, &BulkMarkersDialog::clearMarkersRequested,
            this, [this]() {
        m_mapWidget->clearBulkMarkers();
    });
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void MainWindow::onMapSnapshot() {
    const QPixmap px = m_mapWidget->grabMap();
    if (px.isNull()) return;

    auto* menu = new QMenu(this);
    QAction* copyAct = menu->addAction(appTr("Копіювати як зображення", "Copy as Image"));
    QAction* saveAct = menu->addAction(appTr("Зберегти як зображення...", "Save as Image..."));
    QAction* chosen = menu->exec(QCursor::pos());
    delete menu;

    if (chosen == copyAct) {
        QApplication::clipboard()->setPixmap(px);
    } else if (chosen == saveAct) {
        const QString path = QFileDialog::getSaveFileName(
            this,
            appTr("Зберегти карту", "Save Map"),
            QDir::homePath() + "/map.png",
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty())
            px.save(path);
    }
}
