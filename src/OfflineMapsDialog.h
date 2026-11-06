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
#include <QDialog>
#include "TileDownloader.h"

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QWidget)

// Lets the user configure a map type + zoom range + lat/lon bounding box,
// shows a live tile-count estimate, then downloads and caches the tiles
// under the offline maps folder via TileDownloader.
//
// Two modes:
//  - Simple (default): pick a country/region preset + a named sub-region
//    (or "whole territory") + a zoom preset. Friendly for most users.
//  - Advanced: the raw controls — exact zoom range and lat/lon bounding box.
// Both modes feed the same underlying south/west/north/east/minZoom/maxZoom
// fields, so switching to Advanced starts from whatever the simple picker
// last set, letting the user fine-tune a preset instead of starting over.
class OfflineMapsDialog : public QDialog {
    Q_OBJECT
public:
    explicit OfflineMapsDialog(const QString& basePath, const QString& initialType,
                                QWidget* parent = nullptr);

private slots:
    void onSubRegionChanged();
    void onZoomPresetChanged();
    void onAdvancedModeToggled(bool advanced);
    void updateEstimate();
    void onDownloadClicked();
    void onCancelClicked();
    void onProgress(int done, int total);
    void onFinished(int okCount, int failCount, bool cancelled);

private:
    TileDownloader::Region currentRegion() const;
    void setConfigEnabled(bool enabled);
    // Recomputes the zoom preset combo's entries so every listed preset is
    // actually downloadable (estimated tile count within the hard cap) for
    // the currently selected region + map type — offering a zoom range that
    // then gets blocked as "too many tiles" is exactly what we want to avoid.
    void rebuildZoomPresets();

    QString m_basePath;

    QComboBox*       m_typeCombo;
    QCheckBox*       m_advancedModeCheck;

    // Simple mode
    QWidget*         m_simpleWidget;
    QComboBox*       m_countryCombo;
    QComboBox*       m_subRegionCombo;
    QComboBox*       m_zoomPresetCombo;

    // Advanced mode (raw fields — also the source of truth currentRegion() reads from)
    QWidget*         m_advancedWidget;
    QDoubleSpinBox*  m_south;
    QDoubleSpinBox*  m_west;
    QDoubleSpinBox*  m_north;
    QDoubleSpinBox*  m_east;
    QSpinBox*        m_minZoom;
    QSpinBox*        m_maxZoom;

    QLabel*          m_estimateLbl;
    QLabel*          m_osmWarningLbl;
    QWidget*         m_configWidget;

    QProgressBar*    m_progressBar;
    QLabel*          m_statusLbl;
    QPushButton*     m_downloadBtn;
    QPushButton*     m_cancelBtn;
    QPushButton*     m_closeBtn;

    TileDownloader*  m_downloader = nullptr;
};
