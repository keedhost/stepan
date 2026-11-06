#include "TileDownloader.h"
#include "version.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

int lonToTileX(double lon, int z) {
    const double n = std::pow(2.0, z);
    return static_cast<int>(std::floor((lon + 180.0) / 360.0 * n));
}

int latToTileY(double lat, int z) {
    const double n = std::pow(2.0, z);
    const double latRad = lat * kPi / 180.0;
    return static_cast<int>(std::floor(
        (1.0 - std::asinh(std::tan(latRad)) / kPi) / 2.0 * n));
}

QString userAgent() {
    return QStringLiteral("Stepan/%1 (offline map tile cache; https://github.com/keedhost/stepan)")
        .arg(QString::fromLatin1(APP_VERSION_FULL));
}

} // namespace

TileDownloader::TileDownloader(QObject* parent) : QObject(parent) {
    m_nam = new QNetworkAccessManager(this);
}

qint64 TileDownloader::estimateTileCount(const Region& region) {
    qint64 total = 0;
    for (int z = region.minZoom; z <= region.maxZoom; ++z) {
        const int x1 = lonToTileX(region.west, z);
        const int x2 = lonToTileX(region.east, z);
        const int y1 = latToTileY(region.north, z);
        const int y2 = latToTileY(region.south, z);
        const qint64 w = qAbs(x2 - x1) + 1;
        const qint64 h = qAbs(y2 - y1) + 1;
        total += w * h;
    }
    return total;
}

void TileDownloader::fillQueue() {
    m_pending.clear();
    for (int z = m_region.minZoom; z <= m_region.maxZoom; ++z) {
        const int x1 = lonToTileX(m_region.west, z);
        const int x2 = lonToTileX(m_region.east, z);
        const int y1 = latToTileY(m_region.north, z);
        const int y2 = latToTileY(m_region.south, z);
        const int xMin = qMin(x1, x2), xMax = qMax(x1, x2);
        const int yMin = qMin(y1, y2), yMax = qMax(y1, y2);
        for (int x = xMin; x <= xMax; ++x)
            for (int y = yMin; y <= yMax; ++y)
                m_pending.append({z, x, y});
    }
}

void TileDownloader::start(const Region& region) {
    m_region = region;
    m_cancelled = false;
    m_done = m_ok = m_fail = m_active = 0;
    m_subdomainCounter = 0;

    fillQueue();
    m_total = m_pending.size();
    emit progress(0, m_total);

    if (m_total == 0) {
        emit finished(0, 0, false);
        return;
    }

    for (int i = 0; i < kMaxConcurrent && !m_pending.isEmpty(); ++i)
        downloadNext();
}

void TileDownloader::cancel() {
    m_cancelled = true;
    m_pending.clear();
}

void TileDownloader::downloadNext() {
    if (m_pending.isEmpty()) return;

    const QVector<int> t = m_pending.takeLast();
    const int z = t[0], x = t[1], y = t[2];

    QUrl url;
    if (m_region.typeKey == "ortho") {
        url = QUrl(QStringLiteral(
            "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3")
            .arg(z).arg(y).arg(x));
    } else if (m_region.typeKey == "elevation") {
        // AWS Terrain Tiles, Terrarium encoding (height packed into RGB) —
        // decoded and colored client-side by map.html's elevation layer.
        url = QUrl(QStringLiteral(
            "https://elevation-tiles-prod.s3.amazonaws.com/terrarium/%1/%2/%3.png")
            .arg(z).arg(x).arg(y));
    } else if (m_region.typeKey == "topo") {
        static const char kSub[] = {'a', 'b', 'c'};
        const char sub = kSub[m_subdomainCounter++ % 3];
        url = QUrl(QStringLiteral("https://%1.tile.opentopomap.org/%2/%3/%4.png")
            .arg(sub).arg(z).arg(x).arg(y));
    } else { // streets
        static const char kSub[] = {'a', 'b', 'c'};
        const char sub = kSub[m_subdomainCounter++ % 3];
        url = QUrl(QStringLiteral("https://%1.tile.openstreetmap.org/%2/%3/%4.png")
            .arg(sub).arg(z).arg(x).arg(y));
    }

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", userAgent().toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    ++m_active;
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, z, x, y]() {
        handleReply(reply, z, x, y);
    });
}

void TileDownloader::handleReply(QNetworkReply* reply, int z, int x, int y) {
    reply->deleteLater();
    --m_active;

    if (m_cancelled) {
        if (m_active == 0)
            emit finished(m_ok, m_fail, true);
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        const QString dirPath = QStringLiteral("%1/%2/%3/%4")
            .arg(m_region.basePath, m_region.typeKey).arg(z).arg(x);
        QDir().mkpath(dirPath);
        QFile file(QStringLiteral("%1/%2.png").arg(dirPath).arg(y));
        if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size()) {
            ++m_ok;
        } else {
            ++m_fail;
        }
    } else {
        ++m_fail;
    }
    ++m_done;
    emit progress(m_done, m_total);

    if (!m_pending.isEmpty()) {
        downloadNext();
    } else if (m_active == 0) {
        emit finished(m_ok, m_fail, false);
    }
}
