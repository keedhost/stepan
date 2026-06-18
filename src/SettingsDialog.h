#pragma once
#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onAccepted();
    void updateWebUrls();

private:
    // Запуск
    QCheckBox* m_autoStart      = nullptr;
    QCheckBox* m_startMinimized = nullptr;
    // Поведінка
    QCheckBox* m_closeToTray    = nullptr;
    // Карта
    QComboBox* m_defaultMap     = nullptr;
    // Трей
    QComboBox* m_trayIconStyle  = nullptr;
    // Веб-інтерфейс
    QCheckBox* m_webEnabled     = nullptr;
    QComboBox* m_webAddress     = nullptr;
    QSpinBox*  m_webPort        = nullptr;
    QLabel*    m_webUrlLabel    = nullptr;
    // Мова
    QComboBox* m_language       = nullptr;

    void load();
    void save();
};
