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
    // The address and port the running server was started with (empty / 0
    // when stopped), so callers can tell whether settings changed.
    QString boundHost() const { return m_host; }
    quint16 boundPort() const { return m_port; }

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
    QHash<QTcpSocket*, QByteArray> m_buffers;  // request bytes per open connection
    QString                      m_host;
    quint16                      m_port = 0;

    // Limits against a client (anyone on the LAN when bound to 0.0.0.0)
    // exhausting memory or connections: a request is a short GET, so its
    // headers must fit in kMaxRequestBytes and arrive within kRequestTimeoutMs.
    static constexpr int kMaxRequestBytes  = 8 * 1024;
    static constexpr int kRequestTimeoutMs = 10 * 1000;
    static constexpr int kMaxConnections   = 32;
};
