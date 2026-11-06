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
#include "CoordinateData.h"
#include <QMainWindow>
#include <optional>

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QWidget)

class MapWidget;
class WebServer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Called from TrayManager to pre-fill the input and bring window to front
    void setInputText(const QString& text);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

signals:
    void settingsChanged();

private slots:
    void onInputChanged(const QString& text);
    void onOpenGoogleMaps();
    void onOpenOSM();
    void onMapSourceChanged(const QString& key);
    void onCheckForUpdates();
    void onShowSettings();
    void onShowConsoleHelp();
    void onShowWebHelp();
    void onShowBulkMarkersHelp();
    void onShowPrivacy();
    void onShowAbout();
    void onWebServerStarted(quint16 port);
    void onWebServerStopped();
    void onOpenBulkMarkers();
    void onMapSnapshot();

private:
    // Input
    QLineEdit*   m_input     = nullptr;
    QLabel*      m_formatLbl = nullptr;

    // External map buttons
    QPushButton* m_googleBtn = nullptr;
    QPushButton* m_osmBtn    = nullptr;

    // Coordinate output rows (MGRS, DD, DMS, UTM)
    struct CoordRow {
        QLabel*      nameLbl  = nullptr;
        QLabel*      valueLbl = nullptr;
        QPushButton* copyBtn  = nullptr;
    };
    CoordRow     m_rows[5]; // MGRS, DD, DMS, UTM, USK-2000
    QWidget*     m_coordsPanel = nullptr;

    // Map
    QButtonGroup* m_mapBtnGroup = nullptr;
    MapWidget*    m_mapWidget   = nullptr;

    // Web server
    WebServer*   m_webServer   = nullptr;
    QLabel*      m_webBanner   = nullptr;

    std::optional<CoordinateData> m_current;

    void setupUi();
    void applyStylesheet();
    void updateOutput(const std::optional<CoordinateData>& data);
    void setRow(int i, const QString& value);
    void copyToClipboard(const QString& text);
    void flashCopied(QPushButton* btn);
    void applyWebSettings();
    void applyOfflineMapSettings();
};
