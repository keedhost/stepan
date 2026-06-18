#include "TrayManager.h"
#include "AppLocale.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidgetAction>
#include <QLineEdit>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QIcon>
#include <QApplication>
#include <QTimer>
#include <QSettings>

TrayManager::TrayManager(QObject* parent) : QObject(parent) {
    m_tray = new QSystemTrayIcon(this);
    reloadTrayIcon();
    m_tray->setToolTip(appTr("Конвертер координат", "Coordinate Converter"));

    // ── Context menu ─────────────────────────────────────────────────────────
    m_menu = new QMenu();

    // Widget action: label + QLineEdit for quick coordinate input
    auto* container = new QWidget();
    container->setMinimumWidth(290);
    auto* vl = new QVBoxLayout(container);
    vl->setContentsMargins(10, 8, 10, 6);
    vl->setSpacing(4);

    auto* lbl = new QLabel(appTr("Введіть координати:", "Enter coordinates:"));
    lbl->setStyleSheet("font-weight: bold; font-size: 12px; color: #222;");

    m_coordInput = new QLineEdit();
    m_coordInput->setPlaceholderText("MGRS, DD, DMS, UTM...");
    m_coordInput->setMinimumHeight(28);
    m_coordInput->setStyleSheet(
        "QLineEdit { font-family: monospace; font-size: 12px; "
        "padding: 3px 6px; border: 1px solid #bbb; border-radius: 4px; }"
    );

    auto* hint = new QLabel(appTr("Натисніть Enter для відкриття", "Press Enter to open"));
    hint->setStyleSheet("font-size: 10px; color: #888;");

    vl->addWidget(lbl);
    vl->addWidget(m_coordInput);
    vl->addWidget(hint);

    auto* widgetAction = new QWidgetAction(m_menu);
    widgetAction->setDefaultWidget(container);
    m_menu->addAction(widgetAction);

    m_menu->addSeparator();

    auto* showAction = new QAction(appTr("Показати вікно", "Show Window"), m_menu);
    connect(showAction, &QAction::triggered, this, &TrayManager::showWindowRequested);
    m_menu->addAction(showAction);

    m_menu->addSeparator();

    auto* quitAction = new QAction(appTr("Вихід", "Quit"), m_menu);
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);
    m_menu->addAction(quitAction);

    m_tray->setContextMenu(m_menu);

    // Focus the input field when the menu opens
    connect(m_menu, &QMenu::aboutToShow, this, [this]() {
        m_coordInput->clear();
        // Small delay ensures the widget is fully shown before grabbing focus
        QTimer::singleShot(100, m_coordInput, [this]() {
            m_coordInput->setFocus(Qt::PopupFocusReason);
        });
    });

    connect(m_tray, &QSystemTrayIcon::activated, this, &TrayManager::onTrayActivated);
    connect(m_coordInput, &QLineEdit::returnPressed, this, &TrayManager::onCoordSubmitted);

    m_tray->show();
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
#ifndef Q_OS_MAC
    // On Windows/Linux: left click shows/hides the main window;
    // right click shows the context menu (handled by Qt automatically).
    if (reason == QSystemTrayIcon::Trigger) {
        emit showWindowRequested();
    }
#else
    // On macOS both Trigger and Context activate the menu bar icon;
    // single left-click delegates to showWindowRequested.
    if (reason == QSystemTrayIcon::Trigger) {
        emit showWindowRequested();
    }
#endif
    Q_UNUSED(reason)
}

void TrayManager::onCoordSubmitted() {
    const QString text = m_coordInput->text().trimmed();
    m_coordInput->clear();
    m_menu->hide();
    if (!text.isEmpty()) {
        emit coordinateEntered(text);
    }
}

void TrayManager::reloadTrayIcon() {
    const QString style = QSettings().value("trayIconStyle", "bw").toString();
    const QString pfx = (style == "wb") ? ":/icons/wb/" : ":/icons/bw/";
    QIcon icon;
    icon.addFile(pfx + "16.png",  {16, 16});
    icon.addFile(pfx + "24.png",  {24, 24});
    icon.addFile(pfx + "32.png",  {32, 32});
    icon.addFile(pfx + "64.png",  {64, 64});
    icon.addFile(pfx + "128.png", {128, 128});
    icon.addFile(pfx + "256.png", {256, 256});
    icon.addFile(pfx + "512.png", {512, 512});
    m_tray->setIcon(icon);
}
