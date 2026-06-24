#pragma once
#include "MapWidget.h"
#include <QDialog>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
class BulkMarkersDialog : public QDialog {
    Q_OBJECT
public:
    explicit BulkMarkersDialog(QWidget* parent = nullptr);

signals:
    void markersReady(const QVector<BulkMarker>& markers);
    void clearMarkersRequested();

private slots:
    void rehighlight();
    void onShowOnMap();
    void onDownloadHtml();
    void onClearMap();

private:
    enum class ParseMode { CoordsFirst, LabelFirst };

    ParseMode       m_parseMode      = ParseMode::CoordsFirst;
    QPushButton*    m_coordsFirstBtn = nullptr;
    QPushButton*    m_labelFirstBtn  = nullptr;
    QPlainTextEdit* m_editor         = nullptr;
    QLabel*         m_status         = nullptr;
    QPushButton*    m_showBtn        = nullptr;
    QPushButton*    m_downloadBtn    = nullptr;

    QCheckBox*          m_tableCheckBox  = nullptr;
    QVector<BulkMarker> m_parsed;

    struct ParsedLine {
        double  lat   = 0;
        double  lon   = 0;
        QString label;
        bool    valid = false;
        QString mgrs;
    };
    static ParsedLine parseLineWithLabel(const QString& line, ParseMode mode);
};
