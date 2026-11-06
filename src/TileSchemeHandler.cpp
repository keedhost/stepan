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

#include "TileSchemeHandler.h"
#include <QWebEngineUrlRequestJob>
#include <QFile>
#include <QFileInfo>

TileSchemeHandler::TileSchemeHandler(QObject* parent)
    : QWebEngineUrlSchemeHandler(parent) {}

void TileSchemeHandler::setBasePath(const QString& path) {
    m_basePath = path;
}

void TileSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job) {
    const QString relPath = job->requestUrl().path(); // "/<type>/<z>/<x>/<y>.png"
    if (m_basePath.isEmpty() || relPath.isEmpty()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    const QString filePath = m_basePath + relPath;

    // Reject any path that escapes the offline maps folder.
    if (!QFileInfo(filePath).absoluteFilePath().startsWith(QFileInfo(m_basePath).absoluteFilePath())) {
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }

    auto* file = new QFile(filePath, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    job->reply("image/png", file);
}
