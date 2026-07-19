#include "TileSchemeHandler.h"
#include <QWebEngineUrlRequestJob>
#include <QFile>
#include <QFileInfo>

TileSchemeHandler::TileSchemeHandler(QObject* parent)
    : QWebEngineUrlSchemeHandler(parent) {}

void TileSchemeHandler::setBasePath(const QString& path) {
    m_basePath = path;
}

void TileSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job) {
    const QString relPath = job->requestUrl().path(); // "/<type>/<z>/<x>/<y>.png"
    if (m_basePath.isEmpty() || relPath.isEmpty()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    const QString filePath = m_basePath + relPath;

    // Reject any path that escapes the offline maps folder.
    if (!QFileInfo(filePath).absoluteFilePath().startsWith(QFileInfo(m_basePath).absoluteFilePath())) {
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }

    auto* file = new QFile(filePath, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    job->reply("image/png", file);
}
