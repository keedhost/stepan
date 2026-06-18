#pragma once
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QWebEngineView)

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

    void setCoordinate(double lat, double lon);
    void setZone(double swLat, double swLon, double neLat, double neLon);
    void clearCoordinate();
    void setLayer(const QString& key); // "streets" | "ortho" | "topo"

private:
    QWebEngineView* m_view;
    bool m_loaded = false;

    enum class PendingType { None, Point, Zone };
    struct PendingState {
        PendingType type = PendingType::None;
        double lat = 0, lon = 0;
        double swLat = 0, swLon = 0, neLat = 0, neLon = 0;
    };
    PendingState m_pending;

    void runJs(const QString& js);
};
