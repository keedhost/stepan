#pragma once
#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onAccepted();
    void updateWebUrls();
    void onBrowseOfflinePath();
    void onDownloadOfflineTiles();

private:
    // Запуск
    QCheckBox* m_autoStart      = nullptr;
    QCheckBox* m_startMinimized = nullptr;
    // Поведінка
    QCheckBox* m_disableCloseToTray = nullptr;
    // Карта
    QComboBox* m_defaultMap     = nullptr;
    // Оффлайн карти
    QCheckBox*   m_offlineEnabled = nullptr;
    QLineEdit*   m_offlinePath    = nullptr;
    QPushButton* m_offlineBrowse  = nullptr;
    QPushButton* m_offlineDownload = nullptr;
    QWidget*     m_offlineConfigWidget = nullptr;
    // Трей
    QComboBox* m_trayIconStyle  = nullptr;
    // Веб-інтерфейс
    QCheckBox* m_webEnabled     = nullptr;
    QComboBox* m_webAddress     = nullptr;
    QSpinBox*  m_webPort        = nullptr;
    QLabel*    m_webUrlLabel    = nullptr;
    // Мова
    QComboBox* m_language       = nullptr;
    // Тема
    QComboBox* m_theme          = nullptr;

    void load();
    void save();
};
