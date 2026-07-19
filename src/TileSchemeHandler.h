#pragma once
#include <QWebEngineUrlSchemeHandler>
#include <QString>

// Serves offline map tiles to the embedded Leaflet map via a custom
// "stepan-tile://" URL scheme instead of file:// — Chromium's sandboxing
// hard-blocks "file://" subresource loads from a qrc:// page (verified:
// "Not allowed to load local resource") regardless of the
// LocalContentCanAccessFileUrls setting, so a registered scheme handler is
// the supported way to hand it local bytes.
//
// Requests look like stepan-tile:///<type>/<z>/<x>/<y>.png and are resolved
// against <basePath>/<type>/<z>/<x>/<y>.png on disk.
class TileSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit TileSchemeHandler(QObject* parent = nullptr);

    void setBasePath(const QString& path);

    void requestStarted(QWebEngineUrlRequestJob* job) override;

private:
    QString m_basePath;
};
