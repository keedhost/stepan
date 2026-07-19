#pragma once
#include <QWidget>
#include <QVector>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QWebEngineView)
QT_FORWARD_DECLARE_CLASS(QPixmap)
class TileSchemeHandler;

struct BulkMarker {
    double  lat   = 0;
    double  lon   = 0;
    QString label;
    QString mgrs;
};

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

    void setCoordinate(double lat, double lon);
    void setZone(double swLat, double swLon, double neLat, double neLon);
    void clearCoordinate();
    void setLayer(const QString& key);

    void setBulkMarkers(const QVector<BulkMarker>& markers);
    void clearBulkMarkers();

    // enabled=false clears offline mode (falls back to online tiles).
    // folderPath is the offline maps root, containing <type>/<z>/<x>/<y>.png.
    void setOfflineMode(bool enabled, const QString& folderPath);

    QPixmap grabMap() const;

    static QString exportBulkMarkersHtml(const QVector<BulkMarker>& markers, bool showTable = false);

private:
    QWebEngineView*    m_view;
    TileSchemeHandler* m_tileSchemeHandler;
    bool m_loaded = false;

    enum class PendingType { None, Point, Zone };
    struct PendingState {
        PendingType type = PendingType::None;
        double lat = 0, lon = 0;
        double swLat = 0, swLon = 0, neLat = 0, neLon = 0;
    };
    PendingState m_pending;

    bool    m_pendingOfflineSet     = false;
    bool    m_pendingOfflineEnabled = false;
    QString m_pendingOfflinePath;

    QString m_pendingLayerKey; // empty = nothing pending

    void runJs(const QString& js);
};
