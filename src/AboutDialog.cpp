#include "AboutDialog.h"
#include "AppLocale.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QFrame>
#include <QPixmap>
#include <QFont>

static const char* kBuildDate = __DATE__;  // e.g. "Jun 18 2026"
static const char* kBuildTime = __TIME__;  // e.g. "14:32:05"

// ── HTML helpers ──────────────────────────────────────────────────────────────

static QString aboutHtml() {
    const QString qtVer  = QString::fromLatin1(QT_VERSION_STR);
    const QString build  = QString::fromLatin1(kBuildDate) + "  " +
                           QString::fromLatin1(kBuildTime);

    if (appLangIsEnglish()) return QString(R"(
<html><body style="font-family: sans-serif; font-size: 13px; margin: 6px 2px;">

<h3 style="margin-top:0; color:#1a1a1a;">Author</h3>
<table style="border-spacing: 2px 7px;">
  <tr><td width="110"><b>Name:</b></td>
      <td>Andrii Kondratiev</td></tr>
  <tr><td><b>E-mail:</b></td>
      <td><a href="mailto:h0st@ukr.net">h0st@ukr.net</a></td></tr>
  <tr><td><b>Signal:</b></td>
      <td><a href="https://signal.me/#eu/bFZM1mKCbasBHG5aCjzTaVSWd1ZAOCdo5-eZkmlc8xTNlEnr2r9N-Jnheu5HTTWd">signal.me (open in Signal)</a></td></tr>
  <tr><td><b>WhatsApp:</b></td>
      <td><a href="https://wa.me/380961239268">wa.me/380961239268</a></td></tr>
  <tr><td><b>Facebook:</b></td>
      <td><a href="https://www.facebook.com/kondratyev.andriy">facebook.com/kondratyev.andriy</a></td></tr>
  <tr><td><b>LinkedIn:</b></td>
      <td><a href="https://www.linkedin.com/in/andriy-kondratyev/">linkedin.com/in/andriy-kondratyev</a></td></tr>
  <tr><td><b>Source code:</b></td>
      <td><a href="https://github.com/keedhost/stepan">github.com/keedhost/stepan</a></td></tr>
</table>

<h3 style="color:#1a1a1a;">Technologies</h3>
<table style="border-spacing: 2px 5px;">
  <tr><td width="185"><b>Qt %1</b></td>
      <td>Cross-platform framework (GUI, network)</td></tr>
  <tr><td><b>GeographicLib 2.3</b></td>
      <td>Geodetic coordinate conversion</td></tr>
  <tr><td><b>Leaflet.js 1.9.4</b></td>
      <td>Interactive web maps</td></tr>
  <tr><td><b>OpenStreetMap</b></td>
      <td>Open street map data</td></tr>
  <tr><td><b>Esri World Imagery</b></td>
      <td>Satellite imagery and reference labels</td></tr>
  <tr><td><b>OpenTopoMap</b></td>
      <td>Topographic maps</td></tr>
  <tr><td><b>CMake</b></td>
      <td>Build system</td></tr>
</table>

<p style="color:#888; font-size:12px; margin-top:14px;">
  Build date:&nbsp;<b>%2</b>
</p>
</body></html>)").arg(qtVer, build);

    // Ukrainian
    return QString(R"(
<html><body style="font-family: sans-serif; font-size: 13px; margin: 6px 2px;">

<h3 style="margin-top:0; color:#1a1a1a;">Автор</h3>
<table style="border-spacing: 2px 7px;">
  <tr><td width="130"><b>Ім'я:</b></td>
      <td>Андрій Кондратьєв</td></tr>
  <tr><td><b>E-mail:</b></td>
      <td><a href="mailto:h0st@ukr.net">h0st@ukr.net</a></td></tr>
  <tr><td><b>Signal:</b></td>
      <td><a href="https://signal.me/#eu/bFZM1mKCbasBHG5aCjzTaVSWd1ZAOCdo5-eZkmlc8xTNlEnr2r9N-Jnheu5HTTWd">signal.me (відкрити у Signal)</a></td></tr>
  <tr><td><b>WhatsApp:</b></td>
      <td><a href="https://wa.me/380961239268">wa.me/380961239268</a></td></tr>
  <tr><td><b>Facebook:</b></td>
      <td><a href="https://www.facebook.com/kondratyev.andriy">facebook.com/kondratyev.andriy</a></td></tr>
  <tr><td><b>LinkedIn:</b></td>
      <td><a href="https://www.linkedin.com/in/andriy-kondratyev/">linkedin.com/in/andriy-kondratyev</a></td></tr>
  <tr><td><b>Вихідний код:</b></td>
      <td><a href="https://github.com/keedhost/stepan">github.com/keedhost/stepan</a></td></tr>
</table>

<h3 style="color:#1a1a1a;">Використані технології</h3>
<table style="border-spacing: 2px 5px;">
  <tr><td width="185"><b>Qt %1</b></td>
      <td>Кросплатформний фреймворк (GUI, мережа)</td></tr>
  <tr><td><b>GeographicLib 2.3</b></td>
      <td>Геодезичні перетворення координат</td></tr>
  <tr><td><b>Leaflet.js 1.9.4</b></td>
      <td>Інтерактивні вебкарти</td></tr>
  <tr><td><b>OpenStreetMap</b></td>
      <td>Відкриті картографічні дані</td></tr>
  <tr><td><b>Esri World Imagery</b></td>
      <td>Супутникові знімки та підписи об'єктів</td></tr>
  <tr><td><b>OpenTopoMap</b></td>
      <td>Топографічні карти</td></tr>
  <tr><td><b>CMake</b></td>
      <td>Система збірки</td></tr>
</table>

<p style="color:#888; font-size:12px; margin-top:14px;">
  Дата збірки:&nbsp;<b>%2</b>
</p>
</body></html>)").arg(qtVer, build);
}

static QString licenseHtml() {
    if (appLangIsEnglish()) return R"(
<html><body style="font-family: monospace; font-size: 12px; margin: 6px 2px; white-space: pre-wrap; line-height: 1.55;">
<b>Stepan — Coordinate System Converter</b>
Copyright &copy; 2026 Andrii Kondratiev

<b>GNU GENERAL PUBLIC LICENSE — Version 3</b>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see
<a href="https://www.gnu.org/licenses/gpl-3.0.html">https://www.gnu.org/licenses/gpl-3.0.html</a>

──────────────────────────────────────────────────────

<b>What this means for you:</b>

• You are free to run, study, share and modify this software.
• If you distribute modified versions, you must release them
  under the same GPL-3.0 license and provide the source code.
• You may not impose additional restrictions on the rights
  granted by this license.
• There is NO WARRANTY for this software.

Full license text:
<a href="https://www.gnu.org/licenses/gpl-3.0.html">gnu.org/licenses/gpl-3.0.html</a>
</body></html>)";

    return R"(
<html><body style="font-family: monospace; font-size: 12px; margin: 6px 2px; white-space: pre-wrap; line-height: 1.55;">
<b>Степан — Конвертер систем координат</b>
Copyright &copy; 2026 Андрій Кондратьєв

<b>GNU GENERAL PUBLIC LICENSE — Версія 3</b>

Ця програма є вільним програмним забезпеченням: ви можете
перерозповсюджувати її та/або змінювати відповідно до умов
Стандартної громадської ліцензії GNU, опублікованої Free Software
Foundation, версії 3 або (на ваш вибір) будь-якої пізнішої версії.

Ця програма розповсюджується в надії, що вона буде корисною,
але БЕЗ БУДЬ-ЯКИХ ГАРАНТІЙ; навіть без мається на увазі гарантії
ПРИДАТНОСТІ ДЛЯ ПРОДАЖУ або ПРИДАТНОСТІ ДЛЯ КОНКРЕТНОЇ МЕТИ.
Дивіться Стандартну громадську ліцензію GNU для отримання
додаткової інформації.

Ви повинні були отримати копію Стандартної громадської ліцензії GNU
разом із цією програмою. Якщо ні, перегляньте:
<a href="https://www.gnu.org/licenses/gpl-3.0.html">https://www.gnu.org/licenses/gpl-3.0.html</a>

──────────────────────────────────────────────────────

<b>Що це означає для вас:</b>

• Ви можете вільно запускати, вивчати, поширювати та змінювати
  цю програму.
• Якщо ви поширюєте змінені версії — ви зобов'язані робити це
  на умовах тієї ж GPL-3.0 і надавати вихідний код.
• Ви не можете накладати додаткові обмеження на права,
  надані цією ліцензією.
• На цю програму НЕ НАДАЄТЬСЯ жодних гарантій.

Повний текст ліцензії:
<a href="https://www.gnu.org/licenses/gpl-3.0.html">gnu.org/licenses/gpl-3.0.html</a>
</body></html>)";
}

static QString supportHtml() {
    if (appLangIsEnglish()) return R"(
<html><body style="font-family: sans-serif; font-size: 13px; margin: 6px 2px; line-height: 1.7;">
<p>If Stepan saves you time, you're welcome to say thanks&nbsp;☕</p>

<h3 style="margin-top: 16px; color:#1a1a1a;">PayPal</h3>
<p>
  Open <a href="https://www.paypal.com">paypal.com</a>, choose
  <i>Send &amp; Request</i> and enter:<br>
  <b style="font-size:14px;">h0st@ukr.net</b>
</p>

<h3 style="margin-top: 16px; color:#1a1a1a;">Monobank</h3>
<p>
  <a href="https://send.monobank.ua/AS4vebthC8">send.monobank.ua/AS4vebthC8</a>
</p>

<p style="color:#888; font-size:12px; margin-top:20px;">
  Any amount is appreciated. Thank you!
</p>
</body></html>)";

    return R"(
<html><body style="font-family: sans-serif; font-size: 13px; margin: 6px 2px; line-height: 1.7;">
<p>Якщо Степан заощаджує вам час — завжди можна сказати дякую&nbsp;☕</p>

<h3 style="margin-top: 16px; color:#1a1a1a;">PayPal</h3>
<p>
  Відкрийте <a href="https://www.paypal.com">paypal.com</a>, оберіть
  <i>Надіслати та запросити</i> і введіть:<br>
  <b style="font-size:14px;">h0st@ukr.net</b>
</p>

<h3 style="margin-top: 16px; color:#1a1a1a;">Monobank</h3>
<p>
  <a href="https://send.monobank.ua/AS4vebthC8">send.monobank.ua/AS4vebthC8</a>
</p>

<p style="color:#888; font-size:12px; margin-top:20px;">
  Будь-яка сума приємна. Дякую!
</p>
</body></html>)";
}

// ── Dialog ────────────────────────────────────────────────────────────────────

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(appTr("Про програму", "About Stepan"));
    setMinimumSize(520, 480);
    resize(540, 520);
    setModal(true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 16);
    root->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────────
    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(18);

    auto* iconLbl = new QLabel(this);
    QPixmap px(":/icons/color/128.png");
    iconLbl->setPixmap(px.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLbl->setFixedSize(72, 72);
    headerRow->addWidget(iconLbl, 0, Qt::AlignTop);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(3);

    auto* nameLbl = new QLabel("Степан", this);
    QFont f = nameLbl->font();
    f.setPointSize(22);
    f.setBold(true);
    nameLbl->setFont(f);

    auto* verLbl = new QLabel(appTr("Версія 1.0.0", "Version 1.0.0"), this);
    verLbl->setStyleSheet("color: #777; font-size: 12px;");

    auto* descLbl = new QLabel(
        appTr("Конвертер систем координат\nMGRS · DD · DMS · UTM · УСК-2000",
              "Coordinate System Converter\nMGRS · DD · DMS · UTM · USK-2000"), this);
    descLbl->setStyleSheet("font-size: 13px; color: #333;");

    titleCol->addWidget(nameLbl);
    titleCol->addWidget(verLbl);
    titleCol->addSpacing(5);
    titleCol->addWidget(descLbl);
    titleCol->addStretch();

    headerRow->addLayout(titleCol);
    headerRow->addStretch();
    root->addLayout(headerRow);
    root->addSpacing(14);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    root->addWidget(sep);
    root->addSpacing(10);

    // ── Tabs ──────────────────────────────────────────────────────────────────
    auto* tabs = new QTabWidget(this);

    auto makeBrowser = [this]() {
        auto* b = new QTextBrowser(this);
        b->setOpenExternalLinks(true);
        b->setFrameShape(QFrame::NoFrame);
        b->setStyleSheet("background: transparent;");
        return b;
    };

    auto* aboutBrowser = makeBrowser();
    aboutBrowser->setHtml(aboutHtml());
    tabs->addTab(aboutBrowser, appTr("Про програму", "About"));

    auto* licenseBrowser = makeBrowser();
    licenseBrowser->setHtml(licenseHtml());
    tabs->addTab(licenseBrowser, appTr("Ліцензія", "License"));

    auto* supportBrowser = makeBrowser();
    supportBrowser->setHtml(supportHtml());
    tabs->addTab(supportBrowser, appTr("Допомогти фінансово", "Support the project"));

    root->addWidget(tabs, 1);
    root->addSpacing(10);

    // ── Close button ──────────────────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(btns);
}
