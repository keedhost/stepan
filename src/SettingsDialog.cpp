#include "SettingsDialog.h"
#include "AppLocale.h"
#include "AutoStart.h"
#include "WebServer.h"
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(appTr("Налаштування", "Settings"));
    setMinimumWidth(440);
    setModal(true);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);

    // ── Запуск ────────────────────────────────────────────────────────────────
    auto* launchBox    = new QGroupBox(appTr("Запуск", "Startup"), this);
    auto* launchLayout = new QVBoxLayout(launchBox);

    m_autoStart = new QCheckBox(appTr("Запускати при вході в систему", "Launch at login"), launchBox);
    if (!AutoStart::isSupported()) {
        m_autoStart->setEnabled(false);
        m_autoStart->setToolTip(appTr("Не підтримується на цій платформі", "Not supported on this platform"));
    }
    launchLayout->addWidget(m_autoStart);

    m_startMinimized = new QCheckBox(appTr("Запускати мінімізованим (без відкриття вікна)", "Start minimized (no window)"), launchBox);
    launchLayout->addWidget(m_startMinimized);

    root->addWidget(launchBox);

    // ── Поведінка ─────────────────────────────────────────────────────────────
    auto* behavBox    = new QGroupBox(appTr("Поведінка", "Behavior"), this);
    auto* behavLayout = new QVBoxLayout(behavBox);

    m_closeToTray = new QCheckBox(appTr("При закритті вікна — приховувати у трей (не виходити)", "On close — hide to tray (don't quit)"), behavBox);
    behavLayout->addWidget(m_closeToTray);

    root->addWidget(behavBox);

    // ── Карта ─────────────────────────────────────────────────────────────────
    auto* mapBox  = new QGroupBox(appTr("Карта", "Map"), this);
    auto* mapForm = new QFormLayout(mapBox);

    m_defaultMap = new QComboBox(mapBox);
    m_defaultMap->addItem(appTr("Вулиці (OSM)", "Streets (OSM)"),    "streets");
    m_defaultMap->addItem(appTr("Ортофото (Esri)", "Orthoimage (Esri)"), "ortho");
    m_defaultMap->addItem(appTr("Топографічна", "Topographic"),    "topo");
    mapForm->addRow(appTr("Джерело за замовчуванням:", "Default source:"), m_defaultMap);

    root->addWidget(mapBox);

    // ── Трей ──────────────────────────────────────────────────────────────────
    auto* trayBox  = new QGroupBox(appTr("Трей", "Tray"), this);
    auto* trayForm = new QFormLayout(trayBox);

    m_trayIconStyle = new QComboBox(trayBox);
    m_trayIconStyle->addItem(appTr("Чорна на білому (BW)", "Black on white (BW)"), "bw");
    m_trayIconStyle->addItem(appTr("Біла на чорному (WB)", "White on black (WB)"), "wb");
    trayForm->addRow(appTr("Іконка трею:", "Tray icon:"), m_trayIconStyle);

    root->addWidget(trayBox);

    // ── Веб-інтерфейс ─────────────────────────────────────────────────────────
    auto* webBox    = new QGroupBox(appTr("Веб-інтерфейс", "Web Interface"), this);
    auto* webLayout = new QVBoxLayout(webBox);

    m_webEnabled = new QCheckBox(appTr("Увімкнути вбудований веб-сервер", "Enable built-in web server"), webBox);
    webLayout->addWidget(m_webEnabled);

    auto* webForm     = new QWidget(webBox);
    auto* webFormLyt  = new QFormLayout(webForm);
    webFormLyt->setContentsMargins(0, 4, 0, 0);

    m_webAddress = new QComboBox(webForm);
    m_webAddress->addItem(appTr("Всі мережі (0.0.0.0)", "All networks (0.0.0.0)"),       "0.0.0.0");
    m_webAddress->addItem(appTr("Лише локальний (127.0.0.1)", "Local only (127.0.0.1)"),  "127.0.0.1");
    webFormLyt->addRow(appTr("Мережевий інтерфейс:", "Network interface:"), m_webAddress);

    m_webPort = new QSpinBox(webForm);
    m_webPort->setRange(1024, 65535);
    m_webPort->setValue(8080);
    webFormLyt->addRow(appTr("Порт:", "Port:"), m_webPort);

    webForm->setEnabled(false);
    webLayout->addWidget(webForm);

    m_webUrlLabel = new QLabel(webBox);
    m_webUrlLabel->setWordWrap(true);
    m_webUrlLabel->setOpenExternalLinks(true);
    m_webUrlLabel->setStyleSheet("color: #2a5a9a; font-size: 12px; padding-top: 2px;");
    webLayout->addWidget(m_webUrlLabel);

    root->addWidget(webBox);

    // ── Мова / Language ───────────────────────────────────────────────────────
    auto* langBox  = new QGroupBox(appTr("Мова", "Language"), this);
    auto* langForm = new QFormLayout(langBox);

    m_language = new QComboBox(langBox);
    m_language->addItem("Українська", "uk");
    m_language->addItem("English",    "en");
    langForm->addRow(appTr("Мова інтерфейсу:", "Interface language:"), m_language);

    auto* restartNote = new QLabel(
        appTr("Для застосування мови перезапустіть програму.",
              "Restart the app to apply the language change."), langBox);
    restartNote->setStyleSheet("color: #888; font-size: 11px;");
    langForm->addRow(restartNote);

    root->addWidget(langBox);

    // ── Кнопки ───────────────────────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    // Логіка увімкнення/вимкнення полів веб-сервера
    connect(m_webEnabled, &QCheckBox::toggled, webForm, &QWidget::setEnabled);
    connect(m_webEnabled, &QCheckBox::toggled,   this, &SettingsDialog::updateWebUrls);
    connect(m_webAddress, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::updateWebUrls);
    connect(m_webPort, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::updateWebUrls);

    load();
}

void SettingsDialog::load() {
    QSettings s;
    m_autoStart->setChecked(AutoStart::isEnabled());
    m_startMinimized->setChecked(s.value("startMinimized", false).toBool());
    m_closeToTray->setChecked(s.value("closeToTray", true).toBool());

    const QString map = s.value("defaultMap", "streets").toString();
    const int mapIdx = m_defaultMap->findData(map);
    if (mapIdx >= 0) m_defaultMap->setCurrentIndex(mapIdx);

    const QString trayStyle = s.value("trayIconStyle", "bw").toString();
    const int trayIdx = m_trayIconStyle->findData(trayStyle);
    if (trayIdx >= 0) m_trayIconStyle->setCurrentIndex(trayIdx);

    m_webEnabled->setChecked(s.value("webEnabled", false).toBool());

    const QString addr = s.value("webAddress", "0.0.0.0").toString();
    const int addrIdx = m_webAddress->findData(addr);
    if (addrIdx >= 0) m_webAddress->setCurrentIndex(addrIdx);

    m_webPort->setValue(s.value("webPort", 8080).toInt());

    const QString lang = s.value("language", "uk").toString();
    const int langIdx = m_language->findData(lang);
    if (langIdx >= 0) m_language->setCurrentIndex(langIdx);

    updateWebUrls();
}

void SettingsDialog::save() {
    QSettings s;
    if (AutoStart::isSupported())
        AutoStart::setEnabled(m_autoStart->isChecked());
    s.setValue("startMinimized", m_startMinimized->isChecked());
    s.setValue("closeToTray",    m_closeToTray->isChecked());
    s.setValue("defaultMap",      m_defaultMap->currentData().toString());
    s.setValue("trayIconStyle",   m_trayIconStyle->currentData().toString());
    s.setValue("webEnabled",      m_webEnabled->isChecked());
    s.setValue("webAddress",     m_webAddress->currentData().toString());
    s.setValue("webPort",        m_webPort->value());
    s.setValue("language",       m_language->currentData().toString());
}

void SettingsDialog::updateWebUrls() {
    if (!m_webEnabled->isChecked()) {
        m_webUrlLabel->clear();
        return;
    }
    const QString addr = m_webAddress->currentData().toString();
    const quint16 port = static_cast<quint16>(m_webPort->value());
    const QStringList urls = WebServer::listenUrls(addr, port);

    QStringList links;
    for (const QString& url : urls)
        links << QString("<a href='%1'>%1</a>").arg(url);
    m_webUrlLabel->setText(appTr("Доступно за: ", "Available at: ") + links.join(" &nbsp;·&nbsp; "));
}

void SettingsDialog::onAccepted() {
    save();
    accept();
}
