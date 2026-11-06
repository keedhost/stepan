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
#include <QHash>
#include <QHostAddress>

class QTcpServer;
class QTcpSocket;

class WebServer : public QObject {
    Q_OBJECT
public:
    explicit WebServer(QObject* parent = nullptr);
    ~WebServer();

    bool    start(const QString& host, quint16 port);
    void    stop();
    bool    isRunning() const;
    quint16 actualPort() const;

    // Returns all URLs the server can be reached at given a bind address and port
    static QStringList listenUrls(const QString& boundAddress, quint16 port);

signals:
    void started(quint16 port);
    void stopped();
    void errorOccurred(const QString& message);

private slots:
    void onNewConnection();

private:
    void        handleRequest(QTcpSocket* socket, const QByteArray& request);
    QByteArray  respond(int status, const QByteArray& type, const QByteArray& body);
    QByteArray  serveConvert(const QString& queryString);
    QByteArray  serve404();

    QTcpServer*                  m_server = nullptr;
    QHash<QTcpSocket*, QByteArray> m_buffers;
};
