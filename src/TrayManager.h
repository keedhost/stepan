#pragma once
#include <QObject>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);

signals:
    void coordinateEntered(const QString& text);
    void showWindowRequested();
    void quitRequested();

public slots:
    void reloadTrayIcon();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onCoordSubmitted();

private:
    QSystemTrayIcon* m_tray;
    QMenu*           m_menu;
    QLineEdit*       m_coordInput;
};
