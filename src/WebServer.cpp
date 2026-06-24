#include "WebServer.h"
#include "CoordinateParser.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>

// ── HTML page served at / ─────────────────────────────────────────────────────

static const char* kHtml = R"HTML(<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Степан — Конвертер координат</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
     background:#f0f2f5;min-height:100vh;display:flex;
     align-items:flex-start;justify-content:center;padding:2rem 1rem}
.card{background:#fff;border-radius:12px;padding:2rem;
      width:100%;max-width:620px;box-shadow:0 2px 16px rgba(0,0,0,.1)}
h1{font-size:1.6rem;color:#1a1a2e;margin-bottom:.2rem}
.sub{color:#666;font-size:.9rem;margin-bottom:1.5rem}
.row{display:flex;gap:.5rem;margin-bottom:1.25rem}
input[type=text]{flex:1;padding:.6rem 1rem;border:2px solid #ddd;
                 border-radius:8px;font-size:1rem;font-family:monospace;
                 outline:none;transition:border-color .15s}
input[type=text]:focus{border-color:#3d7cf5}
button.main{padding:.6rem 1.4rem;background:#3d7cf5;color:#fff;border:none;
            border-radius:8px;font-size:1rem;cursor:pointer;transition:background .15s;white-space:nowrap}
button.main:hover{background:#2563d5}
.badge{display:inline-block;font-size:.75rem;background:#e8f0fe;
       color:#3d7cf5;padding:.15rem .55rem;border-radius:4px;margin-bottom:.9rem}
.frow{display:flex;align-items:center;padding:.45rem 0;
      border-bottom:1px solid #f0f0f0}
.frow:last-child{border-bottom:none}
.fname{font-weight:600;font-size:.78rem;color:#555;width:75px;flex-shrink:0}
.fval{font-family:monospace;font-size:.92rem;flex:1}
.cbtn{padding:.2rem .55rem;font-size:.72rem;background:#f4f4f4;
      border:1px solid #ddd;border-radius:4px;cursor:pointer;color:#555;
      margin-left:.4rem;transition:background .15s}
.cbtn:hover{background:#d4f4d4}
.links{margin-top:.9rem;padding-top:.7rem;border-top:1px solid #eee;
       display:flex;gap:.5rem;flex-wrap:wrap}
a.ml{font-size:.82rem;color:#3d7cf5;text-decoration:none;
     padding:.25rem .8rem;border:1px solid #3d7cf5;border-radius:20px;transition:all .15s}
a.ml:hover{background:#3d7cf5;color:#fff}
.err{color:#c44;font-size:.9rem;padding:.3rem 0}
#results{display:none}
</style>
</head>
<body>
<div class="card">
  <h1>Степан</h1>
  <p class="sub">Конвертер систем координат</p>
  <div class="row">
    <input type="text" id="coord" placeholder="Введіть координату…"
           autocomplete="off" autocorrect="off" spellcheck="false"/>
    <button class="main" onclick="go()">Конвертувати</button>
  </div>
  <div class="err" id="err"></div>
  <div id="results">
    <div class="badge" id="badge"></div>
    <div id="rows"></div>
    <div class="links" id="links"></div>
  </div>
</div>
<script>
const KEYS=[['mgrs','MGRS'],['dd','DD'],['dms','DMS'],['utm','UTM'],['usk2000','УСК-2000']];
async function go(){
  const q=document.getElementById('coord').value.trim();
  if(!q)return;
  document.getElementById('err').textContent='';
  document.getElementById('results').style.display='none';
  try{
    const r=await fetch('/api/convert?q='+encodeURIComponent(q));
    const d=await r.json();
    if(!d.success){document.getElementById('err').textContent=d.error||'Помилка';return;}
    show(d);
  }catch(e){document.getElementById('err').textContent='Помилка зʼєднання';}
}
function show(d){
  document.getElementById('badge').textContent=d.format;
  const rows=document.getElementById('rows');
  rows.innerHTML='';
  KEYS.forEach(([k,l])=>{
    const v=(d[k]||'').replace(/'/g,"\\'");
    rows.insertAdjacentHTML('beforeend',
      `<div class="frow"><span class="fname">${l}</span>`+
      `<span class="fval">${d[k]||''}</span>`+
      `<button class="cbtn" onclick="cp('${v}',this)">Копіювати</button></div>`);
  });
  document.getElementById('links').innerHTML=
    `<a class="ml" href="https://www.google.com/maps/search/?api=1&query=${d.lat},${d.lon}" target="_blank">Google Maps</a>`+
    `<a class="ml" href="https://www.openstreetmap.org/?mlat=${d.lat}&mlon=${d.lon}&zoom=14" target="_blank">OpenStreetMap</a>`+
    `<a class="ml" href="https://www.bing.com/maps/?cp=${d.lat}~${d.lon}&lvl=14" target="_blank">Bing Maps</a>`;
  document.getElementById('results').style.display='block';
}
function cp(t,b){
  navigator.clipboard.writeText(t).then(()=>{
    const o=b.textContent;b.textContent='✓';
    setTimeout(()=>b.textContent=o,1200);
  });
}
document.getElementById('coord').addEventListener('keydown',e=>{if(e.key==='Enter')go();});
</script>
</body>
</html>
)HTML";

// ── WebServer implementation ──────────────────────────────────────────────────

WebServer::WebServer(QObject* parent) : QObject(parent) {}

WebServer::~WebServer() {
    QSignalBlocker blocker(this);
    stop();
}

bool WebServer::start(const QString& host, quint16 port) {
    stop();
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &WebServer::onNewConnection);

    const QHostAddress addr = host == "0.0.0.0"
        ? QHostAddress::Any
        : QHostAddress(host);

    if (!m_server->listen(addr, port)) {
        const QString msg = m_server->errorString();
        m_server->deleteLater();
        m_server = nullptr;
        emit errorOccurred(msg);
        return false;
    }
    emit started(m_server->serverPort());
    return true;
}

void WebServer::stop() {
    if (!m_server) return;
    m_server->close();
    m_server->deleteLater();
    m_server = nullptr;
    m_buffers.clear();
    emit stopped();
}

bool WebServer::isRunning() const { return m_server && m_server->isListening(); }
quint16 WebServer::actualPort() const { return m_server ? m_server->serverPort() : 0; }

QStringList WebServer::listenUrls(const QString& boundAddress, quint16 port) {
    QStringList result;
    if (boundAddress == "127.0.0.1") {
        result << QString("http://127.0.0.1:%1/").arg(port);
    } else {
        for (const QHostAddress& a : QNetworkInterface::allAddresses()) {
            if (a.protocol() == QAbstractSocket::IPv4Protocol && !a.isLoopback())
                result << QString("http://%1:%2/").arg(a.toString()).arg(port);
        }
        result << QString("http://127.0.0.1:%1/").arg(port);
    }
    return result;
}

// ── Connection handling ───────────────────────────────────────────────────────

void WebServer::onNewConnection() {
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            m_buffers[socket] += socket->readAll();
            if (m_buffers[socket].contains("\r\n\r\n")) {
                const QByteArray req = m_buffers.take(socket);
                handleRequest(socket, req);
            }
        });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_buffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void WebServer::handleRequest(QTcpSocket* socket, const QByteArray& request) {
    // Parse first line: "GET /path?query HTTP/1.1"
    const int lineEnd = request.indexOf('\r');
    if (lineEnd < 0) { socket->write(serve404()); socket->disconnectFromHost(); return; }

    const auto parts = request.left(lineEnd).split(' ');
    if (parts.size() < 2 || parts[0] != "GET") {
        socket->write(respond(405, "text/plain", "Method Not Allowed"));
        socket->disconnectFromHost();
        return;
    }

    const QString rawPath = QString::fromUtf8(parts[1]);
    const int qmark = rawPath.indexOf('?');
    const QString path  = qmark >= 0 ? rawPath.left(qmark) : rawPath;
    const QString query = qmark >= 0 ? rawPath.mid(qmark + 1) : QString();

    QByteArray body;
    QByteArray type;

    if (path == "/" || path == "/index.html") {
        body = QByteArray(kHtml);
        type = "text/html; charset=utf-8";
    } else if (path == "/api/convert") {
        body = serveConvert(query);
        type = "application/json; charset=utf-8";
    } else {
        socket->write(serve404());
        socket->disconnectFromHost();
        return;
    }

    socket->write(respond(200, type, body));
    socket->disconnectFromHost();
}

QByteArray WebServer::respond(int status, const QByteArray& type, const QByteArray& body) {
    const QByteArray statusText = status == 200 ? "OK"
                                : status == 404 ? "Not Found"
                                : "Error";
    QByteArray r = "HTTP/1.1 " + QByteArray::number(status) + " " + statusText + "\r\n";
    r += "Content-Type: "   + type + "\r\n";
    r += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    r += "Connection: close\r\n";
    r += "Access-Control-Allow-Origin: *\r\n";
    r += "\r\n";
    r += body;
    return r;
}

QByteArray WebServer::serve404() {
    return respond(404, "text/plain", "Not Found");
}

QByteArray WebServer::serveConvert(const QString& queryString) {
    // Decode q= parameter
    QString q;
    for (const QString& param : queryString.split('&')) {
        const int eq = param.indexOf('=');
        if (eq > 0 && param.left(eq) == "q") {
            q = QUrl::fromPercentEncoding(
                    param.mid(eq + 1).replace('+', ' ').toUtf8());
            break;
        }
    }

    if (q.trimmed().isEmpty())
        return R"({"success":false,"error":"Порожній запит"})";

    const auto result = CoordinateParser::parse(q);
    if (!result)
        return R"({"success":false,"error":"Формат не розпізнано"})";

    QJsonObject obj;
    obj["success"]  = true;
    obj["format"]   = CoordinateParser::formatName(result->detectedFormat);
    obj["mgrs"]     = result->mgrs;
    obj["dd"]       = result->dd;
    obj["dms"]      = result->dms;
    obj["utm"]      = result->utm;
    obj["usk2000"]  = result->usk2000;
    obj["lat"]      = result->lat;
    obj["lon"]      = result->lon;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
