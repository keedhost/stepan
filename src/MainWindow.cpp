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
#include <QMenuBar>
#include <QMenu>
#include <QDialog>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QSettings>
#include <QCoreApplication>

static const char* kStyles = R"(
QMainWindow {
    background: #f0f0f0;
}
QLineEdit#coordInput {
    font-size: 14px;
    padding: 8px 12px;
    border: 2px solid #ccc;
    border-radius: 6px;
    background: #fff;
    font-family: monospace;
}
QLineEdit#coordInput:focus {
    border-color: #3d7cf5;
}
QLabel#formatLabel {
    font-size: 12px;
    padding: 2px 4px;
}
QPushButton#externalBtn {
    padding: 6px 14px;
    border: 1px solid #bbb;
    border-radius: 5px;
    background: #fff;
    font-size: 12px;
}
QPushButton#externalBtn:hover  { background: #e8f0fe; border-color: #3d7cf5; }
QPushButton#externalBtn:pressed { background: #c6d5f9; }
QPushButton#externalBtn:disabled { color: #aaa; }
QFrame#coordsPanel {
    background: #fff;
    border: 1px solid #dde;
    border-radius: 6px;
}
QLabel.coordName {
    font-weight: bold;
    font-size: 12px;
    color: #444;
    min-width: 50px;
}
QLabel.coordValue {
    font-family: monospace;
    font-size: 12px;
    color: #111;
    padding: 2px 6px;
}
QPushButton#copyBtn {
    padding: 3px 10px;
    border: 1px solid #ccc;
    border-radius: 4px;
    background: #f8f8f8;
    font-size: 11px;
    color: #555;
    min-width: 70px;
}
QPushButton#copyBtn:hover  { background: #e0ffe0; border-color: #5a5; color: #333; }
QPushButton#copyBtn:pressed { background: #c0f0c0; }
QPushButton#pasteBtn {
    padding: 8px 14px;
    border: 2px solid #bbb;
    border-radius: 6px;
    background: #fff;
    font-size: 14px;
    color: #333;
    white-space: nowrap;
}
QPushButton#pasteBtn:hover  { background: #e8f0fe; border-color: #3d7cf5; color: #1a52c0; }
QPushButton#pasteBtn:pressed { background: #c6d5f9; }
QLabel#mapSourceLabel {
    font-size: 12px;
    color: #555;
}
QPushButton#mapBtnFirst,
QPushButton#mapBtnMid,
QPushButton#mapBtnLast {
    padding: 5px 11px;
    border: 1px solid #bbb;
    border-radius: 0;
    background: #f5f5f5;
    font-size: 12px;
    color: #333;
}
QPushButton#mapBtnFirst {
    border-top-left-radius: 5px;
    border-bottom-left-radius: 5px;
    border-right: none;
}
QPushButton#mapBtnMid {
    border-right: none;
}
QPushButton#mapBtnLast {
    border-top-right-radius: 5px;
    border-bottom-right-radius: 5px;
}
QPushButton#mapBtnFirst:checked,
QPushButton#mapBtnMid:checked,
QPushButton#mapBtnLast:checked {
    background: #3d7cf5;
    color: #ffffff;
    border-color: #2a5fd0;
}
QPushButton#mapBtnFirst:hover:!checked,
QPushButton#mapBtnMid:hover:!checked,
QPushButton#mapBtnLast:hover:!checked {
    background: #e8f0fe;
    border-color: #8ab0f5;
}
)";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Степан — твій друг в системі координат");
    setMinimumSize(620, 700);
    resize(750, 850);
    setStyleSheet(kStyles);
    setupUi();

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
    pasteBtn->setFixedHeight(m_input->sizeHint().height());

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
    m_formatLbl->setStyleSheet("color: #888; font-style: italic; font-size: 12px;");
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

    // --- Separator ---
    auto* sep1 = new QFrame(central);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep1);

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
        m_rows[i].nameLbl->setStyleSheet("font-weight: bold; font-size: 12px; color: #444; min-width: 50px;");
        m_rows[i].nameLbl->setFixedWidth(55);

        m_rows[i].valueLbl = new QLabel("—", rowWidget);
        m_rows[i].valueLbl->setStyleSheet("font-family: monospace; font-size: 12px; color: #111; padding: 2px 6px;");
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
    mapHeader->addStretch();
    root->addLayout(mapHeader);

    // --- Map widget ---
    m_mapWidget = new MapWidget(central);
    m_mapWidget->setMinimumHeight(320);
    m_mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_mapWidget, 1);

    // --- Web server banner ---
    m_webBanner = new QLabel(central);
    m_webBanner->setStyleSheet(
        "background: #e8f4e8; color: #1a6a1a; padding: 4px 10px; "
        "border: 1px solid #9acd9a; border-radius: 4px; font-size: 12px;");
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

void MainWindow::onShowAbout() {
    AboutDialog dlg(this);
    dlg.exec();
}
