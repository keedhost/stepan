#include "MapWidget.h"
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>

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

void MapWidget::setBulkMarkers(const QVector<BulkMarker>& markers) {
    if (!m_loaded) return;
    QJsonArray arr;
    for (const auto& m : markers) {
        QJsonArray item;
        item.append(m.lat);
        item.append(m.lon);
        item.append(m.label);
        item.append(m.mgrs);
        arr.append(item);
    }
    const QString json = QString::fromUtf8(
        QJsonDocument(arr).toJson(QJsonDocument::Compact));
    runJs("setBulkMarkers(" + json + ");");
}

void MapWidget::clearBulkMarkers() {
    runJs("clearBulkMarkers();");
}

QString MapWidget::exportBulkMarkersHtml(const QVector<BulkMarker>& markers, bool showTable) {
    QJsonArray arr;
    for (const auto& m : markers) {
        QJsonArray item;
        item.append(m.lat);
        item.append(m.lon);
        item.append(m.label);
        item.append(m.mgrs);
        arr.append(item);
    }
    const QString json = QString::fromUtf8(
        QJsonDocument(arr).toJson(QJsonDocument::Compact));

    // Badge on right when no table, left when table occupies bottom-right
    const QString badgePos = showTable
        ? "bottom:8px;left:8px;"
        : "bottom:24px;right:8px;";

    const QString tableStyle = showTable ? QString(
        "#coord-tbl{"
          "position:absolute;bottom:8px;right:8px;z-index:1001;"
          "background:rgba(255,255,255,0.55);"
          "backdrop-filter:blur(6px);-webkit-backdrop-filter:blur(6px);"
          "border:1px solid rgba(0,0,0,0.15);border-radius:6px;"
          "max-height:38vh;width:300px;overflow:hidden;"
          "display:flex;flex-direction:column;"
          "font:11px/1.5 sans-serif;"
          "opacity:0.55;transition:opacity .25s;"
          "box-shadow:0 2px 8px rgba(0,0,0,.18);"
        "}"
        "#coord-tbl:hover{opacity:1}"
        "#ct-head{"
          "padding:5px 10px;font-weight:bold;font-size:11px;color:#444;"
          "border-bottom:1px solid rgba(0,0,0,.1);flex-shrink:0;"
          "white-space:nowrap;"
        "}"
        "#ct-scroll{overflow-y:auto;flex:1}"
        "#coord-tbl table{width:100%;border-collapse:collapse}"
        "#coord-tbl tr{cursor:pointer}"
        "#coord-tbl tr:hover{background:rgba(61,124,245,.12)}"
        "#coord-tbl tr.sel{background:rgba(61,124,245,.22)}"
        "#coord-tbl td{"
          "padding:3px 7px;border-bottom:1px solid rgba(0,0,0,.06);"
          "color:#333;vertical-align:middle;overflow:hidden;"
          "max-width:120px;white-space:nowrap;text-overflow:ellipsis;"
        "}"
        "#coord-tbl td:first-child{color:#999;width:22px;text-align:right;flex-shrink:0}"
        "#coord-tbl td code{font-size:10px;font-family:monospace;color:#1a5c9e}"
        "#coord-tbl th{padding:3px 7px;background:rgba(0,0,0,.04);color:#666;font-weight:normal;text-align:left;font-size:10px;border-bottom:1px solid rgba(0,0,0,.1)}"
    ) : QString();

    const QString tableHtml = showTable
        ? "<div id=\"coord-tbl\">"
          "<div id=\"ct-head\">Позначки: " + QString::number(markers.size()) + "</div>"
          "<div id=\"ct-scroll\">"
          "<table><thead><tr><th>#</th><th>MGRS</th><th>Опис</th></tr></thead>"
          "<tbody id=\"ct-rows\"></tbody></table>"
          "</div></div>\n"
        : QString();

    const QString tableJs = showTable ? QString(
        "var ctrows=document.getElementById('ct-rows');\n"
        "mk.forEach(function(marker,i){\n"
        "  var m=data[i];\n"
        "  var tr=document.createElement('tr');\n"
        "  tr.innerHTML='<td>'+(i+1)+'</td>'"
        "    +'<td><code>'+(m[3]||'')+'</code></td>'"
        "    +'<td>'+(m[2]||'')+'</td>';\n"
        "  tr.addEventListener('click',function(){\n"
        "    marker.openPopup();\n"
        "    map.setView(marker.getLatLng(),Math.max(map.getZoom(),14));\n"
        "    ctrows.querySelectorAll('tr.sel').forEach(function(r){r.classList.remove('sel');});\n"
        "    tr.classList.add('sel');\n"
        "    tr.scrollIntoView({block:'nearest'});\n"
        "  });\n"
        "  ctrows.appendChild(tr);\n"
        "});\n"
    ) : QString();

    QString html =
        "<!DOCTYPE html>\n"
        "<html lang=\"uk\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\">\n"
        "<title>Позначки (" + QString::number(markers.size()) + ") — Степан</title>\n"
        "<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\"/>\n"
        "<style>"
          "*{margin:0;padding:0;box-sizing:border-box}html,body,#map{width:100%;height:100%}"
          "#stepan-badge{"
            "position:absolute;" + badgePos +
            "z-index:1000;"
            "background:rgba(255,255,255,0.88);backdrop-filter:blur(4px);"
            "border:1px solid #ddd;border-radius:4px;"
            "padding:4px 9px;font:11px/1.4 sans-serif;color:#444;"
            "pointer-events:auto;white-space:nowrap;"
            "box-shadow:0 1px 4px rgba(0,0,0,.15);"
          "}"
          "#stepan-badge a{color:#2a6ad4;text-decoration:none}"
          "#stepan-badge a:hover{text-decoration:underline}" +
          tableStyle +
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<div id=\"map\"></div>\n"
        "<div id=\"stepan-badge\">Зроблено <a href=\"https://keedhost.github.io/stepan/\" target=\"_blank\">Степаном</a></div>\n" +
        tableHtml +
        "<script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\"></script>\n"
        "<script>\n"
        "var map=L.map('map');\n"
        "L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',{\n"
        "  attribution:'&copy; <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors',\n"
        "  maxZoom:19\n"
        "}).addTo(map);\n"
        "var data=" + json + ";\n"
        "var mk=data.map(function(m){\n"
        "  var marker=L.marker([m[0],m[1]]).addTo(map);\n"
        "  var p=m[2]?'<b>'+m[2]+'</b><br>':'';\n"
        "  if(m[3])p+='<code style=\"font-size:12px\">'+m[3]+'</code><br>';\n"
        "  p+='<small>'+m[0].toFixed(6)+', '+m[1].toFixed(6)+'</small>';\n"
        "  marker.bindPopup(p);\n"
        "  return marker;\n"
        "});\n"
        "if(mk.length===1){map.setView([data[0][0],data[0][1]],14);}\n"
        "else if(mk.length>1){map.fitBounds(L.featureGroup(mk).getBounds().pad(0.15));}\n"
        "else{map.setView([50,31],7);}\n" +
        tableJs +
        "</script>\n"
        "</body>\n"
        "</html>\n";
    return html;
}
