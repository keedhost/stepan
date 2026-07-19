#include "OfflineMapsDialog.h"
#include "AppLocale.h"
#include "RegionPresets.h"
#include "Theme.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPoint>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace {
constexpr qint64 kWarnTileCount = 3000;
constexpr qint64 kMaxTileCount  = 100000;
// A z4 tile is ~22.5° wide, so requesting anything below z6 (~5.6° wide)
// pulls in tiles reaching hundreds of km beyond the selected region — e.g.
// a z4 tile touching Zakarpattia's western edge spans all the way to the
// Atlantic coast. z6 keeps the "overview" tier honest to the chosen region.
constexpr int kZoomFloor = 6;

int providerMaxZoom(const QString& typeKey) {
    return typeKey == "topo" ? 17 : 19;
}

// Highest zoom <= desiredMaxZoom for which the region still fits under cap.
int feasibleMaxZoom(TileDownloader::Region region, int desiredMaxZoom, qint64 cap) {
    region.minZoom = kZoomFloor;
    for (int z = desiredMaxZoom; z > kZoomFloor; --z) {
        region.maxZoom = z;
        if (TileDownloader::estimateTileCount(region) <= cap) return z;
    }
    return kZoomFloor;
}
}

OfflineMapsDialog::OfflineMapsDialog(const QString& basePath, const QString& initialType,
                                     QWidget* parent)
    : QDialog(parent), m_basePath(basePath) {
    setWindowTitle(appTr("Завантаження оффлайн-карт", "Download Offline Maps"));
    setMinimumWidth(460);
    setModal(true);

    auto* root = new QVBoxLayout(this);

    m_configWidget = new QWidget(this);
    auto* form = new QFormLayout(m_configWidget);

    m_typeCombo = new QComboBox(m_configWidget);
    m_typeCombo->addItem(appTr("Вулиці (OSM)", "Streets (OSM)"),       "streets");
    m_typeCombo->addItem(appTr("Ортофото (Esri)", "Orthoimage (Esri)"), "ortho");
    m_typeCombo->addItem(appTr("Топографічна", "Topographic"),         "topo");
    const int typeIdx = m_typeCombo->findData(initialType);
    if (typeIdx >= 0) m_typeCombo->setCurrentIndex(typeIdx);
    form->addRow(appTr("Тип карти:", "Map type:"), m_typeCombo);

    m_osmWarningLbl = new QLabel(appTr(
        "OpenStreetMap блокує масове завантаження тайлів зі своїх серверів "
        "(перевірено: систематичне завантаження регіону повертає помилку 403). "
        "Для оффлайн-кешу надійніше працюють «Ортофото (Esri)» або «Топографічна».",
        "OpenStreetMap blocks bulk tile downloads from its servers "
        "(verified: a systematic region download returns a 403 error). "
        "\"Orthoimage (Esri)\" or \"Topographic\" work more reliably for an offline cache."),
        m_configWidget);
    m_osmWarningLbl->setWordWrap(true);
    m_osmWarningLbl->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::colors().danger));
    form->addRow(m_osmWarningLbl);

    m_advancedModeCheck = new QCheckBox(appTr("Розширений режим", "Advanced mode"), m_configWidget);
    form->addRow(m_advancedModeCheck);

    // ── Simple mode: region + sub-region + zoom presets ────────────────────────
    m_simpleWidget = new QWidget(m_configWidget);
    auto* simpleForm = new QFormLayout(m_simpleWidget);
    simpleForm->setContentsMargins(0, 0, 0, 0);

    m_countryCombo = new QComboBox(m_simpleWidget);
    for (const CountryPreset& c : RegionPresets::all())
        m_countryCombo->addItem(c.name);
    simpleForm->addRow(appTr("Регіон:", "Region:"), m_countryCombo);

    m_subRegionCombo = new QComboBox(m_simpleWidget);
    simpleForm->addRow(appTr("Область:", "Area:"), m_subRegionCombo);

    m_zoomPresetCombo = new QComboBox(m_simpleWidget);
    simpleForm->addRow(appTr("Масштаб:", "Zoom:"), m_zoomPresetCombo);

    form->addRow(m_simpleWidget);

    // ── Advanced mode: raw zoom range + lat/lon bounding box ───────────────────
    m_advancedWidget = new QWidget(m_configWidget);
    auto* advForm = new QFormLayout(m_advancedWidget);
    advForm->setContentsMargins(0, 0, 0, 0);

    auto* zoomRow = new QHBoxLayout();
    m_minZoom = new QSpinBox(m_advancedWidget);
    m_minZoom->setRange(1, 19);
    m_minZoom->setValue(6);
    m_maxZoom = new QSpinBox(m_advancedWidget);
    m_maxZoom->setRange(1, 19);
    m_maxZoom->setValue(13);
    zoomRow->addWidget(m_minZoom);
    zoomRow->addWidget(new QLabel("–", m_advancedWidget));
    zoomRow->addWidget(m_maxZoom);
    zoomRow->addStretch();
    advForm->addRow(appTr("Масштаб (min–max):", "Zoom (min–max):"), zoomRow);

    auto makeCoordSpin = [this](double lo, double hi, double val) {
        auto* sb = new QDoubleSpinBox(m_advancedWidget);
        sb->setRange(lo, hi);
        sb->setDecimals(2);
        sb->setSingleStep(0.5);
        sb->setValue(val);
        return sb;
    };
    // Default region: Ukraine's approximate bounding box.
    m_south = makeCoordSpin(-90, 90, 44.0);
    m_north = makeCoordSpin(-90, 90, 52.5);
    m_west  = makeCoordSpin(-180, 180, 22.0);
    m_east  = makeCoordSpin(-180, 180, 40.5);

    auto* latRow = new QHBoxLayout();
    latRow->addWidget(m_south);
    latRow->addWidget(new QLabel("–", m_advancedWidget));
    latRow->addWidget(m_north);
    advForm->addRow(appTr("Широта (Пд–Пн):", "Latitude (S–N):"), latRow);

    auto* lonRow = new QHBoxLayout();
    lonRow->addWidget(m_west);
    lonRow->addWidget(new QLabel("–", m_advancedWidget));
    lonRow->addWidget(m_east);
    advForm->addRow(appTr("Довгота (Зх–Сх):", "Longitude (W–E):"), lonRow);

    form->addRow(m_advancedWidget);
    m_advancedWidget->setVisible(false);

    m_estimateLbl = new QLabel(m_configWidget);
    m_estimateLbl->setWordWrap(true);
    form->addRow(appTr("Оцінка:", "Estimate:"), m_estimateLbl);

    root->addWidget(m_configWidget);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    root->addWidget(m_progressBar);

    m_statusLbl = new QLabel(this);
    m_statusLbl->setWordWrap(true);
    m_statusLbl->setVisible(false);
    root->addWidget(m_statusLbl);

    auto* btnRow = new QHBoxLayout();
    m_downloadBtn = new QPushButton(appTr("Завантажити", "Download"), this);
    m_cancelBtn   = new QPushButton(appTr("Скасувати завантаження", "Cancel Download"), this);
    m_cancelBtn->setVisible(false);
    m_closeBtn    = new QPushButton(appTr("Закрити", "Close"), this);
    btnRow->addWidget(m_downloadBtn);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_closeBtn);
    root->addLayout(btnRow);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OfflineMapsDialog::rebuildZoomPresets);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OfflineMapsDialog::updateEstimate);
    connect(m_advancedModeCheck, &QCheckBox::toggled, this, &OfflineMapsDialog::onAdvancedModeToggled);

    connect(m_countryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_subRegionCombo->clear();
        const auto& countries = RegionPresets::all();
        const int idx = m_countryCombo->currentIndex();
        if (idx < 0 || idx >= countries.size()) return;
        for (const SubRegion& sr : countries[idx].subRegions)
            m_subRegionCombo->addItem(sr.name);
    });
    connect(m_subRegionCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OfflineMapsDialog::onSubRegionChanged);
    connect(m_zoomPresetCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OfflineMapsDialog::onZoomPresetChanged);

    connect(m_minZoom,   QOverload<int>::of(&QSpinBox::valueChanged),         this, &OfflineMapsDialog::updateEstimate);
    connect(m_maxZoom,   QOverload<int>::of(&QSpinBox::valueChanged),         this, &OfflineMapsDialog::updateEstimate);
    connect(m_south, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &OfflineMapsDialog::updateEstimate);
    connect(m_north, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &OfflineMapsDialog::updateEstimate);
    connect(m_west,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &OfflineMapsDialog::updateEstimate);
    connect(m_east,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &OfflineMapsDialog::updateEstimate);

    connect(m_downloadBtn, &QPushButton::clicked, this, &OfflineMapsDialog::onDownloadClicked);
    connect(m_cancelBtn,   &QPushButton::clicked, this, &OfflineMapsDialog::onCancelClicked);
    connect(m_closeBtn,    &QPushButton::clicked, this, &QDialog::reject);

    // Populate sub-region combo for the (only, for now) default country, then
    // apply the default zoom preset — both drive the underlying spin boxes.
    if (!RegionPresets::all().isEmpty()) {
        for (const SubRegion& sr : RegionPresets::all().first().subRegions)
            m_subRegionCombo->addItem(sr.name);
    }
    onSubRegionChanged();
    updateEstimate();
}

void OfflineMapsDialog::onSubRegionChanged() {
    const auto& countries = RegionPresets::all();
    const int countryIdx = m_countryCombo->currentIndex();
    const int subIdx = m_subRegionCombo->currentIndex();
    if (countryIdx < 0 || countryIdx >= countries.size()) return;
    const auto& subRegions = countries[countryIdx].subRegions;
    if (subIdx < 0 || subIdx >= subRegions.size()) return;

    const SubRegion& sr = subRegions[subIdx];
    m_south->setValue(sr.south);
    m_west->setValue(sr.west);
    m_north->setValue(sr.north);
    m_east->setValue(sr.east);
    rebuildZoomPresets();
    updateEstimate();
}

void OfflineMapsDialog::onZoomPresetChanged() {
    const QPoint z = m_zoomPresetCombo->currentData().toPoint();
    if (z.x() == 0 && z.y() == 0) return;
    m_minZoom->setValue(z.x());
    m_maxZoom->setValue(z.y());
    updateEstimate();
}

void OfflineMapsDialog::rebuildZoomPresets() {
    const int keepTierIndex = qMax(0, m_zoomPresetCombo->currentIndex());

    TileDownloader::Region base;
    base.typeKey  = m_typeCombo->currentData().toString();
    base.basePath = m_basePath;
    base.south = qMin(m_south->value(), m_north->value());
    base.north = qMax(m_south->value(), m_north->value());
    base.west  = qMin(m_west->value(), m_east->value());
    base.east  = qMax(m_west->value(), m_east->value());

    const int providerMax = providerMaxZoom(base.typeKey);
    const int desired[4] = { qMin(8, providerMax), qMin(11, providerMax), qMin(13, providerMax), providerMax };
    const char* ukLabels[4] = {
        "Огляд (масштаб %1–%2)", "Стандартний (масштаб %1–%2)",
        "Детальний (масштаб %1–%2)", "Максимально детальний (масштаб %1–%2)"
    };
    const char* enLabels[4] = {
        "Overview (zoom %1–%2)", "Standard (zoom %1–%2)",
        "Detailed (zoom %1–%2)", "Maximum detail (zoom %1–%2)"
    };

    m_zoomPresetCombo->blockSignals(true);
    m_zoomPresetCombo->clear();
    for (int i = 0; i < 4; ++i) {
        const int z = feasibleMaxZoom(base, desired[i], kMaxTileCount);
        // Skip a tier that duplicates the previous one exactly (small regions
        // hit the provider's max zoom well before "Detailed"/"Maximum" differ).
        if (m_zoomPresetCombo->count() > 0 &&
            m_zoomPresetCombo->itemData(m_zoomPresetCombo->count() - 1).toPoint().y() == z)
            continue;
        const QString label = appTr(ukLabels[i], enLabels[i]).arg(kZoomFloor).arg(z);
        m_zoomPresetCombo->addItem(label, QVariant::fromValue(QPoint(kZoomFloor, z)));
    }
    m_zoomPresetCombo->setCurrentIndex(qMin(keepTierIndex, m_zoomPresetCombo->count() - 1));
    m_zoomPresetCombo->blockSignals(false);

    onZoomPresetChanged();
}

void OfflineMapsDialog::onAdvancedModeToggled(bool advanced) {
    m_simpleWidget->setVisible(!advanced);
    m_advancedWidget->setVisible(advanced);
}

TileDownloader::Region OfflineMapsDialog::currentRegion() const {
    TileDownloader::Region r;
    r.typeKey  = m_typeCombo->currentData().toString();
    r.basePath = m_basePath;
    r.south    = qMin(m_south->value(), m_north->value());
    r.north    = qMax(m_south->value(), m_north->value());
    r.west     = qMin(m_west->value(), m_east->value());
    r.east     = qMax(m_west->value(), m_east->value());
    r.minZoom  = qMin(m_minZoom->value(), m_maxZoom->value());
    r.maxZoom  = qMax(m_minZoom->value(), m_maxZoom->value());
    return r;
}

void OfflineMapsDialog::updateEstimate() {
    m_osmWarningLbl->setVisible(m_typeCombo->currentData().toString() == "streets");

    const qint64 count = TileDownloader::estimateTileCount(currentRegion());
    const Theme::Colors tc = Theme::colors();
    if (count > kWarnTileCount) {
        m_estimateLbl->setStyleSheet(QString("color: %1;").arg(tc.danger));
        m_estimateLbl->setText(appTr(
            "%1 тайлів — забагато. Зменшіть регіон або діапазон масштабу.",
            "%1 tiles — too many. Shrink the region or zoom range.").arg(count));
    } else {
        m_estimateLbl->setStyleSheet(QString("color: %1;").arg(tc.textMuted));
        m_estimateLbl->setText(appTr("%1 тайлів", "%1 tiles").arg(count));
    }
    m_downloadBtn->setEnabled(count > 0 && count <= kMaxTileCount);
}

void OfflineMapsDialog::setConfigEnabled(bool enabled) {
    m_configWidget->setEnabled(enabled);
    m_closeBtn->setEnabled(enabled);
}

void OfflineMapsDialog::onDownloadClicked() {
    const TileDownloader::Region region = currentRegion();
    const qint64 count = TileDownloader::estimateTileCount(region);
    if (count > kWarnTileCount) {
        const bool proceed = askAppYesNo(this,
            appTr("Багато тайлів", "Many tiles"),
            appTr("Буде завантажено приблизно %1 тайлів. Це може зайняти багато часу та місця на диску. Продовжити?",
                  "About %1 tiles will be downloaded. This may take a long time and disk space. Continue?")
                .arg(count));
        if (!proceed) return;
    }

    setConfigEnabled(false);
    m_downloadBtn->setVisible(false);
    m_cancelBtn->setVisible(true);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_statusLbl->setVisible(true);
    m_statusLbl->setText(appTr("Завантаження…", "Downloading…"));

    m_downloader = new TileDownloader(this);
    connect(m_downloader, &TileDownloader::progress, this, &OfflineMapsDialog::onProgress);
    connect(m_downloader, &TileDownloader::finished,  this, &OfflineMapsDialog::onFinished);
    m_downloader->start(region);
}

void OfflineMapsDialog::onCancelClicked() {
    if (m_downloader) {
        m_cancelBtn->setEnabled(false);
        m_statusLbl->setText(appTr("Скасування…", "Cancelling…"));
        m_downloader->cancel();
    }
}

void OfflineMapsDialog::onProgress(int done, int total) {
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(done);
    m_statusLbl->setText(appTr("Завантажено %1 з %2 тайлів…", "Downloaded %1 of %2 tiles…")
        .arg(done).arg(total));
}

void OfflineMapsDialog::onFinished(int okCount, int failCount, bool cancelled) {
    m_cancelBtn->setVisible(false);
    m_cancelBtn->setEnabled(true);
    m_downloadBtn->setVisible(true);
    setConfigEnabled(true);

    if (cancelled) {
        m_statusLbl->setText(appTr("Скасовано. Завантажено %1 тайлів.", "Cancelled. Downloaded %1 tiles.").arg(okCount));
    } else if (failCount > 0) {
        m_statusLbl->setText(appTr("Готово: %1 завантажено, %2 не вдалося.", "Done: %1 downloaded, %2 failed.")
            .arg(okCount).arg(failCount));
    } else {
        m_statusLbl->setText(appTr("Готово: %1 тайлів завантажено.", "Done: %1 tiles downloaded.").arg(okCount));
    }

    m_downloader->deleteLater();
    m_downloader = nullptr;
}
