#include "MapWidget.h"
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QUrl>

MapWidget::MapWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_view = new QWebEngineView(this);

    // Allow loading external tile servers from a qrc:// origin
    m_view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    m_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    connect(m_view, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (!ok) return;
        m_loaded = true;
        if (m_pending.type == PendingType::Point) {
            setCoordinate(m_pending.lat, m_pending.lon);
        } else if (m_pending.type == PendingType::Zone) {
            setZone(m_pending.swLat, m_pending.swLon, m_pending.neLat, m_pending.neLon);
        }
        m_pending.type = PendingType::None;
    });

    m_view->setUrl(QUrl("qrc:/map.html"));
    layout->addWidget(m_view);
}

void MapWidget::runJs(const QString& js) {
    if (!m_loaded) return;
    m_view->page()->runJavaScript(js);
}

void MapWidget::setCoordinate(double lat, double lon) {
    if (!m_loaded) {
        m_pending = {PendingType::Point, lat, lon};
        return;
    }
    runJs(QString("setCoordinate(%1, %2);")
        .arg(lat, 0, 'f', 8)
        .arg(lon, 0, 'f', 8));
}

void MapWidget::setZone(double swLat, double swLon, double neLat, double neLon) {
    if (!m_loaded) {
        m_pending = {PendingType::Zone, 0, 0, swLat, swLon, neLat, neLon};
        return;
    }
    runJs(QString("setZone(%1, %2, %3, %4);")
        .arg(swLat, 0, 'f', 8).arg(swLon, 0, 'f', 8)
        .arg(neLat, 0, 'f', 8).arg(neLon, 0, 'f', 8));
}

void MapWidget::clearCoordinate() {
    m_pending.type = PendingType::None;
    runJs("clearCoordinate();");
}

void MapWidget::setLayer(const QString& key) {
    runJs(QString("setLayer('%1');").arg(key));
}
