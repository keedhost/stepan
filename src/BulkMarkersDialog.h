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
#include "MapWidget.h"
#include <QDialog>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
class BulkMarkersDialog : public QDialog {
    Q_OBJECT
public:
    explicit BulkMarkersDialog(QWidget* parent = nullptr);

signals:
    void markersReady(const QVector<BulkMarker>& markers);
    void clearMarkersRequested();

private slots:
    void rehighlight();
    void onShowOnMap();
    void onDownloadHtml();
    void onClearMap();

private:
    enum class ParseMode { CoordsFirst, LabelFirst };

    ParseMode       m_parseMode      = ParseMode::CoordsFirst;
    QPushButton*    m_coordsFirstBtn = nullptr;
    QPushButton*    m_labelFirstBtn  = nullptr;
    QPlainTextEdit* m_editor         = nullptr;
    QLabel*         m_status         = nullptr;
    QPushButton*    m_showBtn        = nullptr;
    QPushButton*    m_downloadBtn    = nullptr;

    QCheckBox*          m_tableCheckBox  = nullptr;
    QVector<BulkMarker> m_parsed;

    struct ParsedLine {
        double  lat   = 0;
        double  lon   = 0;
        QString label;
        bool    valid = false;
        QString mgrs;
    };
    static ParsedLine parseLineWithLabel(const QString& line, ParseMode mode);
};
