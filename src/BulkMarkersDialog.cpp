#include "BulkMarkersDialog.h"
#include "CoordinateParser.h"

#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTextEdit>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextBlock>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>

BulkMarkersDialog::BulkMarkersDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Масові позначки на карті");
    setMinimumSize(620, 580);
    resize(680, 640);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(14, 12, 14, 12);

    // ── Help ─────────────────────────────────────────────────────────────────
    auto* helpBox = new QFrame(this);
    helpBox->setFrameShape(QFrame::StyledPanel);
    helpBox->setStyleSheet(
        "QFrame { background:#f0f6ff; border:1px solid #c8daf5; border-radius:4px; }"
        "QLabel { background:transparent; border:none; }");
    auto* helpLayout = new QVBoxLayout(helpBox);
    helpLayout->setContentsMargins(10, 8, 10, 8);
    helpLayout->setSpacing(4);

    auto* helpTitle = new QLabel("<b>Формат рядка:</b> &lt;координати&gt; [роздільник] [підпис]", helpBox);
    helpTitle->setWordWrap(true);
    helpTitle->setStyleSheet("font-size:12px; color:#1a3a6a;");

    auto* helpText = new QLabel(
        "<span style='font-family:monospace; font-size:11px; color:#333;'>"
        "37U CP 94712 95805 - Точка А<br>"
        "37UCP9471295805 Точка А<br>"
        "37U CP 94712 95805: точка А<br>"
        "48.706391°N, 37.568890°E - Точка А<br>"
        "48.70639, 37.56889 Точка А<br>"
        "48°01'15\"N 37°48'36\"E - Позиція<br>"
        "37N 411286 5319318 КП<br>"
        "53-21447 74-11251 Ціль"
        "</span>"
        "<span style='font-size:11px; color:#666;'><br>"
        "Роздільники: «&nbsp;-&nbsp;» або «:&nbsp;» або пробіл. Підпис — необов'язковий.</span>",
        helpBox);
    helpText->setWordWrap(true);

    helpLayout->addWidget(helpTitle);
    helpLayout->addWidget(helpText);
    root->addWidget(helpBox);

    // ── Order toggle ──────────────────────────────────────────────────────────
    auto* toggleRow = new QHBoxLayout();
    toggleRow->setSpacing(0);

    const QString btnBase =
        "QPushButton{"
          "padding:5px 18px;font-size:12px;"
          "border:1px solid #b0b8c4;background:#f5f5f5;color:#333;"
        "}"
        "QPushButton:checked{"
          "background:#3d7cf5;color:#fff;border-color:#3d7cf5;"
        "}"
        "QPushButton:hover:!checked{background:#e8eef8;}";

    auto* coordsFirstBtn = new QPushButton("Спочатку координати", this);
    auto* labelFirstBtn  = new QPushButton("Спочатку опис",        this);
    coordsFirstBtn->setCheckable(true);
    labelFirstBtn->setCheckable(true);
    coordsFirstBtn->setChecked(true);
    coordsFirstBtn->setStyleSheet(btnBase +
        "QPushButton{border-radius:4px 0 0 4px;}");
    labelFirstBtn->setStyleSheet(btnBase +
        "QPushButton{border-radius:0 4px 4px 0;border-left:none;}");

    auto* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(coordsFirstBtn, 0);
    modeGroup->addButton(labelFirstBtn,  1);
    modeGroup->setExclusive(true);

    m_coordsFirstBtn = coordsFirstBtn;
    m_labelFirstBtn  = labelFirstBtn;

    toggleRow->addWidget(coordsFirstBtn);
    toggleRow->addWidget(labelFirstBtn);
    toggleRow->addStretch();
    root->addLayout(toggleRow);

    connect(modeGroup, &QButtonGroup::idToggled, this, [this](int, bool checked) {
        if (!checked) return;
        m_parseMode = m_coordsFirstBtn->isChecked()
            ? ParseMode::CoordsFirst : ParseMode::LabelFirst;
        // Update placeholder to match selected mode
        m_editor->setPlaceholderText(
            m_parseMode == ParseMode::CoordsFirst
            ? "37U CP 94712 95805 - Точка А\n48.70639, 37.56889 Точка Б\n37N 411286 5319318"
            : "Точка А - 37U CP 94712 95805\nТочка Б 48.70639, 37.56889\nКП 37N 411286 5319318");
        rehighlight();
    });

    // ── Editor label ─────────────────────────────────────────────────────────
    auto* editorLbl = new QLabel("Координати (одна позначка — один рядок):", this);
    editorLbl->setStyleSheet("font-weight:bold; font-size:12px;");
    root->addWidget(editorLbl);

    // ── Editor ───────────────────────────────────────────────────────────────
    m_editor = new QPlainTextEdit(this);
    m_editor->setPlaceholderText(
        "37U CP 94712 95805 - Точка А\n"
        "48.70639, 37.56889 Точка Б\n"
        "37N 411286 5319318");
    m_editor->setFont(QFont("Courier New", 12));
    m_editor->setMinimumHeight(200);
    root->addWidget(m_editor, 1);

    // ── Status ───────────────────────────────────────────────────────────────
    m_status = new QLabel("Введіть координати вище", this);
    m_status->setStyleSheet("font-size:12px; color:#666; padding:2px 0;");
    root->addWidget(m_status);

    // ── Table option
    m_tableCheckBox = new QCheckBox(
        "Показати таблицю MGRS-координат у завантаженому HTML-файлі", this);
    m_tableCheckBox->setStyleSheet("font-size:12px; color:#333; padding:2px 0;");
    root->addWidget(m_tableCheckBox);

    // ── Separator ────────────────────────────────────────────────────────────
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep);

    // ── Buttons ──────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_showBtn = new QPushButton("Показати на карті", this);
    m_showBtn->setStyleSheet(
        "QPushButton { padding:7px 16px; border:1px solid #3d7cf5; border-radius:5px;"
        "  background:#3d7cf5; color:#fff; font-size:13px; font-weight:bold; }"
        "QPushButton:hover { background:#2a5fd0; }"
        "QPushButton:disabled { background:#aaa; border-color:#aaa; }");
    m_showBtn->setEnabled(false);

    m_downloadBtn = new QPushButton("Завантажити HTML", this);
    m_downloadBtn->setStyleSheet(
        "QPushButton { padding:7px 16px; border:1px solid #2a7a2a; border-radius:5px;"
        "  background:#fff; color:#2a7a2a; font-size:13px; }"
        "QPushButton:hover { background:#e8f4e8; }"
        "QPushButton:disabled { color:#aaa; border-color:#aaa; }");
    m_downloadBtn->setEnabled(false);

    auto* clearMapBtn = new QPushButton("Очистити карту", this);
    clearMapBtn->setStyleSheet(
        "QPushButton { padding:7px 16px; border:1px solid #bbb; border-radius:5px;"
        "  background:#fff; color:#555; font-size:13px; }"
        "QPushButton:hover { background:#fff0f0; border-color:#c44; color:#c44; }");

    auto* closeBtn = new QPushButton("Закрити", this);
    closeBtn->setStyleSheet(
        "QPushButton { padding:7px 16px; border:1px solid #bbb; border-radius:5px;"
        "  background:#f5f5f5; color:#333; font-size:13px; }"
        "QPushButton:hover { background:#e8e8e8; }");

    btnRow->addWidget(m_showBtn);
    btnRow->addWidget(m_downloadBtn);
    btnRow->addWidget(clearMapBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_editor,      &QPlainTextEdit::textChanged, this, &BulkMarkersDialog::rehighlight);
    connect(m_showBtn,     &QPushButton::clicked, this, &BulkMarkersDialog::onShowOnMap);
    connect(m_downloadBtn, &QPushButton::clicked, this, &BulkMarkersDialog::onDownloadHtml);
    connect(clearMapBtn,   &QPushButton::clicked, this, &BulkMarkersDialog::onClearMap);
    connect(closeBtn,      &QPushButton::clicked, this, &QDialog::accept);
}

// ── Static: parse one line into coordinate + label ────────────────────────────

BulkMarkersDialog::ParsedLine BulkMarkersDialog::parseLineWithLabel(const QString& rawLine,
                                                                    ParseMode mode) {
    const QString line = rawLine.trimmed();
    if (line.isEmpty()) return {};

    // Helper: parse coordPart, strip leading separator char from labelPart
    auto tryParse = [](const QString& coordPart, const QString& labelRest) -> ParsedLine {
        auto r = CoordinateParser::parse(coordPart.trimmed());
        if (!r) return {};
        QString label = labelRest.trimmed();
        if (!label.isEmpty() && (label[0] == '-' || label[0] == ':'))
            label = label.mid(1).trimmed();
        return {r->lat, r->lon, label, true, r->mgrs};
    };

    if (mode == ParseMode::CoordsFirst) {
        // 1. " - " separator: left=coords, right=label
        int idx = line.indexOf(" - ");
        if (idx > 0)
            if (auto r = tryParse(line.left(idx), line.mid(idx + 3)); r.valid) return r;

        // 2. ": " separator
        idx = line.indexOf(": ");
        if (idx > 0)
            if (auto r = tryParse(line.left(idx), line.mid(idx + 2)); r.valid) return r;

        // 3. Full line (coords only, no label)
        if (auto r = CoordinateParser::parse(line))
            return {r->lat, r->lon, QString(), true, r->mgrs};

        // 4. Strip tokens from end: growing coord prefix
        const QStringList tok = line.split(' ', Qt::SkipEmptyParts);
        for (int n = tok.size() - 1; n >= 1; --n)
            if (auto r = tryParse(QStringList(tok.mid(0, n)).join(' '),
                                  QStringList(tok.mid(n)).join(' ')); r.valid) return r;

    } else { // LabelFirst
        // 1. " - " separator: left=label, right=coords
        int idx = line.indexOf(" - ");
        if (idx > 0)
            if (auto r = tryParse(line.mid(idx + 3), line.left(idx)); r.valid) return r;

        // 2. ": " separator
        idx = line.indexOf(": ");
        if (idx > 0)
            if (auto r = tryParse(line.mid(idx + 2), line.left(idx)); r.valid) return r;

        // 3. Full line (coords only, no label)
        if (auto r = CoordinateParser::parse(line))
            return {r->lat, r->lon, QString(), true, r->mgrs};

        // 4. Strip tokens from start: growing label prefix, rest = coords
        const QStringList tok = line.split(' ', Qt::SkipEmptyParts);
        for (int n = 1; n < tok.size(); ++n)
            if (auto r = CoordinateParser::parse(QStringList(tok.mid(n)).join(' ')))
                return {r->lat, r->lon, QStringList(tok.mid(0, n)).join(' '), true, r->mgrs};
    }

    return {};
}

// ── Re-parse and re-highlight on every text change ───────────────────────────

void BulkMarkersDialog::rehighlight() {
    m_parsed.clear();

    QList<QTextEdit::ExtraSelection> errorSels;

    QTextBlock block = m_editor->document()->begin();
    int validCount = 0, errorCount = 0;

    while (block.isValid()) {
        const QString text = block.text();
        if (!text.trimmed().isEmpty()) {
            ParsedLine pl = parseLineWithLabel(text, m_parseMode);
            if (pl.valid) {
                m_parsed.append({pl.lat, pl.lon, pl.label, pl.mgrs});
                ++validCount;
            } else {
                QTextEdit::ExtraSelection sel;
                sel.format.setBackground(QColor("#ffe0e0"));
                sel.format.setProperty(QTextFormat::FullWidthSelection, true);
                QTextCursor cursor(block);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                sel.cursor = cursor;
                errorSels.append(sel);
                ++errorCount;
            }
        }
        block = block.next();
    }

    m_editor->setExtraSelections(errorSels);

    if (validCount == 0 && errorCount == 0) {
        m_status->setText("Введіть координати вище");
        m_status->setStyleSheet("font-size:12px; color:#666; padding:2px 0;");
    } else if (errorCount == 0) {
        m_status->setText(QString("✓  %1 позначок — всі коректні").arg(validCount));
        m_status->setStyleSheet("font-size:12px; color:#2a7a2a; padding:2px 0;");
    } else if (validCount == 0) {
        m_status->setText(QString("✗  %1 помилок — жодної коректної позначки").arg(errorCount));
        m_status->setStyleSheet("font-size:12px; color:#c44; padding:2px 0;");
    } else {
        m_status->setText(QString("⚠  %1 позначок  ·  %2 помилок (червоним)")
            .arg(validCount).arg(errorCount));
        m_status->setStyleSheet("font-size:12px; color:#a06000; padding:2px 0;");
    }

    const bool hasValid = !m_parsed.isEmpty();
    m_showBtn->setEnabled(hasValid);
    m_downloadBtn->setEnabled(hasValid);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void BulkMarkersDialog::onShowOnMap() {
    if (m_parsed.isEmpty()) return;
    emit markersReady(m_parsed);
}

void BulkMarkersDialog::onClearMap() {
    emit clearMarkersRequested();
}

void BulkMarkersDialog::onDownloadHtml() {
    if (m_parsed.isEmpty()) return;

    const QString defaultName = QString("markers_%1.html")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"));

    const QString path = QFileDialog::getSaveFileName(
        this,
        "Зберегти карту з позначками",
        defaultName,
        "HTML файли (*.html)");

    if (path.isEmpty()) return;

    const QString html = MapWidget::exportBulkMarkersHtml(m_parsed, m_tableCheckBox->isChecked());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Помилка",
            QString("Не вдалося зберегти файл:\n%1").arg(path));
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << html;
    file.close();

    QMessageBox::information(this, "Збережено",
        QString("Файл збережено:\n%1\n\nВідкрийте його у браузері для перегляду карти.").arg(path));
}
