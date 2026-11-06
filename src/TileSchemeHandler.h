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
#include <QWebEngineUrlSchemeHandler>
#include <QString>

// Serves offline map tiles to the embedded Leaflet map via a custom
// "stepan-tile://" URL scheme instead of file:// — Chromium's sandboxing
// hard-blocks "file://" subresource loads from a qrc:// page (verified:
// "Not allowed to load local resource") regardless of the
// LocalContentCanAccessFileUrls setting, so a registered scheme handler is
// the supported way to hand it local bytes.
//
// Requests look like stepan-tile:///<type>/<z>/<x>/<y>.png and are resolved
// against <basePath>/<type>/<z>/<x>/<y>.png on disk.
class TileSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit TileSchemeHandler(QObject* parent = nullptr);

    void setBasePath(const QString& path);

    void requestStarted(QWebEngineUrlRequestJob* job) override;

private:
    QString m_basePath;
};
