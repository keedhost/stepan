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
