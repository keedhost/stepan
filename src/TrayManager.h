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
#include <QObject>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);

signals:
    void coordinateEntered(const QString& text);
    void showWindowRequested();
    void quitRequested();

public slots:
    void reloadTrayIcon();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onCoordSubmitted();

private:
    QSystemTrayIcon* m_tray;
    QMenu*           m_menu;
    QLineEdit*       m_coordInput;
};
