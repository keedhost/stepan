#pragma once
#include <QObject>
#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

// Downloads XYZ map tiles for a lat/lon bounding box + zoom range and caches
// them on disk as <basePath>/<typeKey>/<z>/<x>/<y>.png — regardless of the
// tile numbering scheme the source server actually uses (Esri's ortho tiles
// are fetched as {z}/{y}/{x}), so the on-disk layout is always the same
// standard XYZ convention MapWidget's offline tile layer expects.
class TileDownloader : public QObject {
    Q_OBJECT
public:
    struct Region {
        QString typeKey;   // "streets" | "ortho" | "topo" | "elevation"
        QString basePath;  // offline maps root folder
        double south = 0, west = 0, north = 0, east = 0;
        int minZoom = 1, maxZoom = 1;
    };

    explicit TileDownloader(QObject* parent = nullptr);

    // Number of tiles that would be downloaded for this region (no network access).
    static qint64 estimateTileCount(const Region& region);

    void start(const Region& region);
    void cancel();

signals:
    void progress(int done, int total);
    void finished(int okCount, int failCount, bool cancelled);

private:
    void fillQueue();
    void downloadNext();
    void handleReply(QNetworkReply* reply, int z, int x, int y);

    QNetworkAccessManager* m_nam;
    Region m_region;
    QVector<QVector<int>> m_pending; // {z, x, y}
    int m_total = 0;
    int m_done = 0;
    int m_ok = 0;
    int m_fail = 0;
    int m_active = 0;
    int m_subdomainCounter = 0;
    bool m_cancelled = false;

    static constexpr int kMaxConcurrent = 6;
};
