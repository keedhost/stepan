#include "TrayManager.h"
#include "AppLocale.h"
#include "Theme.h"

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
#include <QCursor>

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

    const Theme::Colors tc = Theme::colors();

    auto* lbl = new QLabel(appTr("Введіть координати:", "Enter coordinates:"));
    lbl->setStyleSheet(QString("font-weight: bold; font-size: 12px; color: %1;").arg(tc.text));

    m_coordInput = new QLineEdit();
    m_coordInput->setPlaceholderText("MGRS, DD, DMS, UTM...");
    m_coordInput->setMinimumHeight(28);
    m_coordInput->setStyleSheet(QString(
        "QLineEdit { font-family: monospace; font-size: 12px; "
        "padding: 3px 6px; border: 1px solid %1; border-radius: 4px; "
        "background: %2; color: %3; }"
    ).arg(tc.border, tc.inputBg, tc.text));

    auto* hint = new QLabel(appTr("Натисніть Enter для відкриття", "Press Enter to open"));
    hint->setStyleSheet(QString("font-size: 10px; color: %1;").arg(tc.textMuted));

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

#ifndef Q_OS_MAC
    // On Windows/Linux the native context menu path is safe; let Qt handle it.
    m_tray->setContextMenu(m_menu);
#endif
    // On macOS the menu is shown manually via popup() in onTrayActivated().
    // Attaching it as the native context menu triggers a crash in some Qt/AppKit
    // versions: NSStatusItem's own tracking session calls -[NSEvent clickCount]
    // on a non-mouse event, and AppKit throws an uncaught NSException, aborting
    // the process. Showing the QMenu ourselves avoids that native tracking path.

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
    // On macOS: any click on the icon just shows the menu (popped up
    // manually to avoid the native NSStatusItem tracking-session crash —
    // see the comment above setContextMenu()). The main window only opens
    // via the "Показати вікно" menu action or by submitting a coordinate,
    // never directly from clicking the tray icon.
    Q_UNUSED(reason)
    // QSystemTrayIcon::geometry() is unreliable on macOS (Qt/Cocoa often
    // reports a stale or wrong rect while still passing isValid()), so
    // anchor the popup to the cursor position instead — the click always
    // happens right on the icon.
    m_menu->popup(QCursor::pos());
#endif
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
    const QString style = QSettings().value("trayIconStyle", "wb").toString();
    const QString pfx = (style == "bw") ? ":/icons/bw/" : ":/icons/wb/";
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
