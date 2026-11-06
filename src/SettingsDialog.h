/*
 * Stepan — твій друг в системі координат
 * Copyright (C) 2026 Andrii Kondratiev
 *
 * Слава Україні! Слава ЗСУ!
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
