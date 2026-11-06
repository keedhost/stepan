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

#include "CoordinateParser.h"
#include "AppLocale.h"
#include <GeographicLib/MGRS.hpp>
#include <GeographicLib/UTMUPS.hpp>
#include <GeographicLib/DMS.hpp>
#include <GeographicLib/TransverseMercator.hpp>
#include <GeographicLib/Constants.hpp>
#include <QRegularExpression>
#include <cmath>

std::optional<CoordinateData> CoordinateParser::parse(const QString& input) {
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) return std::nullopt;

    if (auto r = tryMGRS(trimmed))   return r;
    if (auto r = tryUSK2000(trimmed)) return r;
    if (auto r = tryUTM(trimmed))    return r;
    if (auto r = tryDMS(trimmed))    return r;
    if (auto r = tryDD(trimmed))     return r;

    return std::nullopt;
}

// "37UCP8888494707"  →  "37U CP 88884 94707"
static QString formatMGRS(const QString& raw) {
    int i = 0;
    while (i < raw.size() && raw[i].isDigit()) ++i;
    if (i == 0 || i > 2 || i + 3 > raw.size()) return raw;

    const QString gzdNum  = raw.left(i);          // "37"
    const QString band    = raw.mid(i, 1);         // "U"
    const QString grid    = raw.mid(i + 1, 2);     // "CP"
    const QString num     = raw.mid(i + 3);        // "8888494707"

    if (num.isEmpty())          return gzdNum + band + " " + grid;
    if (num.size() % 2 != 0)   return raw;

    const int half = num.size() / 2;
    return gzdNum + band + " " + grid + " " + num.left(half) + " " + num.right(half);
}

// 5396866.3  →  "53-96866"
static QString dashCoord(double v) {
    const QString s = QString::number(static_cast<long long>(std::round(v)));
    if (s.size() < 3) return s;
    return s.left(2) + "-" + s.mid(2);
}

CoordinateData CoordinateParser::buildFromLatLon(double lat, double lon, CoordinateFormat format) {
    CoordinateData data;
    data.lat = lat;
    data.lon = lon;
    data.detectedFormat = format;

    // DD
    data.dd = QString("%1%2, %3%4")
        .arg(std::abs(lat), 0, 'f', 6)
        .arg(lat >= 0 ? "°N" : "°S")
        .arg(std::abs(lon), 0, 'f', 6)
        .arg(lon >= 0 ? "°E" : "°W");

    // DMS helper
    auto toDMS = [](double deg, bool isLat) -> QString {
        const char hemi = (deg >= 0) ? (isLat ? 'N' : 'E') : (isLat ? 'S' : 'W');
        deg = std::abs(deg);
        const int d = static_cast<int>(deg);
        const double mf = (deg - d) * 60.0;
        const int m = static_cast<int>(mf);
        const double s = (mf - m) * 60.0;
        return QString("%1°%2'%3\"%4")
            .arg(d)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 5, 'f', 2, QChar('0'))
            .arg(hemi);
    };
    data.dms = toDMS(lat, true) + " " + toDMS(lon, false);

    // MGRS
    try {
        int zone;
        bool northp;
        double x, y;
        GeographicLib::UTMUPS::Forward(lat, lon, zone, northp, x, y);
        std::string mgrsStr;
        GeographicLib::MGRS::Forward(zone, northp, x, y, lat, 5, mgrsStr);
        data.mgrs = formatMGRS(QString::fromStdString(mgrsStr));
    } catch (...) {
        data.mgrs = "N/A";
    }

    // UTM
    try {
        int zone;
        bool northp;
        double x, y;
        GeographicLib::UTMUPS::Forward(lat, lon, zone, northp, x, y);
        data.utm = QString("%1%2 %3 %4")
            .arg(zone)
            .arg(northp ? 'N' : 'S')
            .arg(static_cast<long long>(std::round(x)))
            .arg(static_cast<long long>(std::round(y)));
    } catch (...) {
        data.utm = "N/A";
    }

    // УСК-2000 (Gauss-Kruger, k₀=1.0, GRS-80 ≈ WGS-84)
    try {
        int gkZone;
        double X, Y;
        latLonToUSK2000(lat, lon, X, Y, gkZone);
        data.usk2000 = dashCoord(X) + " " + dashCoord(Y);
    } catch (...) {
        data.usk2000 = "N/A";
    }

    return data;
}

// ── USK-2000 helpers ─────────────────────────────────────────────────────────

static const GeographicLib::TransverseMercator& gkProjection() {
    // GRS-80 and WGS-84 differ only in the 10th significant figure of the
    // flattening, so using WGS-84 constants gives sub-millimetre accuracy.
    static const GeographicLib::TransverseMercator kGK(
        GeographicLib::Constants::WGS84_a(),
        GeographicLib::Constants::WGS84_f(),
        1.0  // scale factor = 1.0 (Gauss-Kruger) vs 0.9996 (UTM)
    );
    return kGK;
}

void CoordinateParser::latLonToUSK2000(double lat, double lon,
                                        double& X, double& Y, int& zone) {
    // Zone from longitude: zone N covers [6(N-1)°, 6N°), CM = 6N-3 degrees
    zone = static_cast<int>(std::floor(lon / 6.0)) + 1;
    if (zone < 1)  zone = 1;
    if (zone > 60) zone = 60;

    const double lon0 = 6.0 * zone - 3.0;
    double easting, northing;
    gkProjection().Forward(lon0, lat, lon, easting, northing);

    // Soviet/Ukrainian convention: X = northing, Y = easting with zone prefix
    // Y = zone * 1,000,000 + 500,000 + easting_from_CM
    X = northing;
    Y = zone * 1000000.0 + 500000.0 + easting;
}

bool CoordinateParser::usk2000ToLatLon(double X, double Y, double& lat, double& lon) {
    // Extract zone from the leading digit(s) of Y
    const int zone = static_cast<int>(Y / 1000000.0);
    if (zone < 1 || zone > 60) return false;

    const double lon0    = 6.0 * zone - 3.0;
    const double easting = Y - zone * 1000000.0 - 500000.0;
    const double northing = X;

    // Sanity: easting should be within ±400 km of central meridian
    if (std::abs(easting) > 400000.0) return false;
    // Northing: valid for northern hemisphere
    if (northing < 0.0 || northing > 10000000.0) return false;

    try {
        gkProjection().Reverse(lon0, easting, northing, lat, lon);
        if (!(lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0)) return false;

        // USK-2000 is defined for Ukraine only (≈44.4–52.4°N, 22.1–40.2°E);
        // accept that area with a margin. Without this, swapped X/Y almost
        // always decode to some valid-looking point (e.g. "6342000 5592000"
        // landed near Pskov instead of Kyiv) and the swap fallback never ran.
        if (lat < 43.0 || lat > 53.5 || lon < 21.0 || lon > 41.5) return false;
        // The point must lie in the zone named by Y's prefix (6° strip), with
        // the usual ~1° overlap allowed at zone edges.
        if (lon < lon0 - 4.0 || lon > lon0 + 4.0) return false;
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<CoordinateData> CoordinateParser::tryMGRS(const QString& input) {
    // Strip spaces — MGRS is valid with or without spaces
    const QString mgrs = input.simplified().remove(' ');

    // Quick sanity: must start with 1-2 digits, then a letter, then 2 letters, then even digits
    static const QRegularExpression mgrsRe("^\\d{1,2}[A-Za-z][A-Za-z]{2}\\d{0,10}$");
    if (!mgrsRe.match(mgrs).hasMatch()) return std::nullopt;

    try {
        int zone;
        bool northp;
        double x, y;
        int prec;
        GeographicLib::MGRS::Reverse(mgrs.toStdString(), zone, northp, x, y, prec, true);

        double lat, lon;
        GeographicLib::UTMUPS::Reverse(zone, northp, x, y, lat, lon);

        if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) return std::nullopt;

        auto data = buildFromLatLon(lat, lon, CoordinateFormat::MGRS);
        // Preserve original input precision, apply space formatting
        data.mgrs = [&]{
            std::string raw;
            GeographicLib::MGRS::Forward(zone, northp, x, y, lat, prec, raw);
            return formatMGRS(QString::fromStdString(raw));
        }();

        // Zone/area support: for prec < 5 compute the bounding box of the MGRS square
        data.mgrsPrecision = prec;
        if (prec < 5) {
            // x,y are the CENTER of the square (centerp=true above)
            const double side = std::pow(10.0, 5 - prec); // metres per side
            const double half = side / 2.0;
            GeographicLib::UTMUPS::Reverse(zone, northp, x - half, y - half,
                                           data.zoneSWLat, data.zoneSWLon);
            GeographicLib::UTMUPS::Reverse(zone, northp, x + half, y + half,
                                           data.zoneNELat, data.zoneNELon);
        }

        return data;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<CoordinateData> CoordinateParser::tryUTM(const QString& input) {
    // UTM: <zone 1-2 digits><N or S or band letter> <easting 5-8 digits> <northing 1-8 digits>
    // Northing may be short: just north of the equator it is a few metres
    // ("37N 255144 55306"), and the app writes it that way.
    static const QRegularExpression re(
        R"(^(\d{1,2})\s*([A-Za-z])\s+(\d{5,8}(?:\.\d+)?)\s+(\d{1,8}(?:\.\d+)?)$)"
    );
    const auto match = re.match(input.trimmed());
    if (!match.hasMatch()) return std::nullopt;

    const int zone = match.captured(1).toInt();
    const QChar bandChar = match.captured(2).toUpper()[0];
    const double easting  = match.captured(3).toDouble();
    const double northing = match.captured(4).toDouble();

    if (zone < 1 || zone > 60) return std::nullopt;

    // The letter is either the hemisphere or an MGRS latitude band:
    //  - N / S are the hemisphere. That is how this app writes UTM ("56S …" is
    //    Sydney), so its own output must read back. (N as a band, 0–8°N, is
    //    north either way; S as a band, 32–40°N, is read as the hemisphere.)
    //  - Other letters are latitude bands: C–M south, P–X north (no I/O). The
    //    result is checked against the band, so a wrong letter or a wrong
    //    northing is rejected instead of silently landing somewhere else.
    //  - Anything else (A, B, I, O, Y, Z) is not valid UTM and is rejected.
    static const QString kBands = QStringLiteral("CDEFGHJKLMNPQRSTUVWX");
    const bool hemisphereLetter = (bandChar == 'N' || bandChar == 'S');
    const int bandIdx = kBands.indexOf(bandChar);  // 0 = C (80°S … 72°S)
    if (!hemisphereLetter && bandIdx < 0) return std::nullopt;
    const bool northp = hemisphereLetter ? (bandChar == 'N') : (bandChar >= 'N');

    try {
        double lat, lon;
        GeographicLib::UTMUPS::Reverse(zone, northp, easting, northing, lat, lon);

        if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) return std::nullopt;

        if (!hemisphereLetter) {
            // Bands are 8° tall from 80°S; X is 12° (72–84°N). A little slack
            // for points written with the neighbouring band at a boundary.
            const double south = -80.0 + 8.0 * bandIdx;
            const double north = (bandChar == 'X') ? 84.0 : south + 8.0;
            const double slack = 0.5;
            if (lat < south - slack || lat > north + slack) return std::nullopt;
        }

        return buildFromLatLon(lat, lon, CoordinateFormat::UTM);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<CoordinateData> CoordinateParser::tryDMS(const QString& input) {
    // Try splitting into two parts and passing to GeographicLib DMS parser
    // GeographicLib::DMS::DecodeLatLon takes two separate strings
    static const QRegularExpression splitRe(
        R"pat(^([NSns]?\s*\d[^NSnsEWew]*[NSns])\s+([EWew]?\s*\d[^NSnsEWew]*[EWew])$)pat"
    );
    auto splitMatch = splitRe.match(input);
    if (splitMatch.hasMatch()) {
        try {
            double lat, lon;
            GeographicLib::DMS::DecodeLatLon(
                splitMatch.captured(1).toStdString(),
                splitMatch.captured(2).toStdString(),
                lat, lon);
            if (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0)
                return buildFromLatLon(lat, lon, CoordinateFormat::DMS);
        } catch (...) {}
    }

    // Regex fallback for common format: 50°27'00.36"N 30°31'24.24"E
    static const QRegularExpression re(
        "^(\\d{1,3})[\\x{00B0}\\s]\\s*(\\d{1,2})['\\x{2018}\\x{2019}\\s]"
        "\\s*(\\d{1,2}(?:\\.\\d+)?)[\\x{201C}\\x{201D}\\x22\\s]*([NSns])\\s+"
        "(\\d{1,3})[\\x{00B0}\\s]\\s*(\\d{1,2})['\\x{2018}\\x{2019}\\s]"
        "\\s*(\\d{1,2}(?:\\.\\d+)?)[\\x{201C}\\x{201D}\\x22\\s]*([EWew])$"
    );
    const auto match = re.match(input);
    if (!match.hasMatch()) return std::nullopt;

    double lat = match.captured(1).toDouble()
               + match.captured(2).toDouble() / 60.0
               + match.captured(3).toDouble() / 3600.0;
    double lon = match.captured(5).toDouble()
               + match.captured(6).toDouble() / 60.0
               + match.captured(7).toDouble() / 3600.0;

    if (match.captured(4).toUpper() == "S") lat = -lat;
    if (match.captured(8).toUpper() == "W") lon = -lon;

    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) return std::nullopt;

    return buildFromLatLon(lat, lon, CoordinateFormat::DMS);
}

std::optional<CoordinateData> CoordinateParser::tryDD(const QString& input) {
    // Patterns: "50.4501, 30.5234"  "50.4501N 30.5234E"  "N50.4501 E30.5234"  "-50.4501 30.5234"
    static const QRegularExpression re(
        R"(^([NSns])?\s*([+-]?\d{1,3}(?:\.\d+)?)\s*°?\s*([NSns])?\s*[,\s]+\s*([EWew])?\s*([+-]?\d{1,3}(?:\.\d+)?)\s*°?\s*([EWew])?$)"
    );
    const auto match = re.match(input.trimmed());
    if (!match.hasMatch()) return std::nullopt;

    double lat = match.captured(2).toDouble();
    double lon = match.captured(5).toDouble();

    const QString latHemiPre  = match.captured(1).toUpper();
    const QString latHemiPost = match.captured(3).toUpper();
    const QString lonHemiPre  = match.captured(4).toUpper();
    const QString lonHemiPost = match.captured(6).toUpper();

    const QString latHemi = latHemiPre.isEmpty() ? latHemiPost : latHemiPre;
    const QString lonHemi = lonHemiPre.isEmpty() ? lonHemiPost : lonHemiPre;

    if (latHemi == "S") lat = -std::abs(lat);
    if (lonHemi == "W") lon = -std::abs(lon);

    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) return std::nullopt;

    return buildFromLatLon(lat, lon, CoordinateFormat::DD);
}

std::optional<CoordinateData> CoordinateParser::tryUSK2000(const QString& input) {
    const QString s = input.trimmed();

    auto extractDouble = [](const QString& str) -> double {
        return QString(str).replace(',', '.').toDouble();
    };

    auto tryXY = [](double X, double Y) -> std::optional<CoordinateData> {
        double lat, lon;
        if (!CoordinateParser::usk2000ToLatLon(X, Y, lat, lon)) return std::nullopt;
        return CoordinateParser::buildFromLatLon(lat, lon, CoordinateFormat::USK2000);
    };

    // ── Dashed format: "53-96866 73-88840"  (X then Y) ───────────────────────
    static const QRegularExpression reDashed(
        R"(^(\d{2})-(\d+)\s+(\d{2})-(\d+)$)"
    );
    // Unlabeled input: first number is X (northing), second Y (easting with
    // zone prefix). The swapped order is tried too; with the Ukraine-only
    // domain check it is usually unambiguous. When both orders give a point
    // in Ukraine (possible in zone 5), keep "first = X" and say so.
    auto tryEitherOrder = [&](double first, double second) -> std::optional<CoordinateData> {
        auto asXY = tryXY(first, second);
        auto asYX = tryXY(second, first);
        if (asXY && asYX
            && (std::abs(asXY->lat - asYX->lat) > 1e-5 || std::abs(asXY->lon - asYX->lon) > 1e-5)) {
            asXY->note = appTr(
                "Порядок X/Y неоднозначний: перше число прочитано як X (північ). "
                "Якщо це Y — поміняйте числа місцями.",
                "Ambiguous X/Y order: the first number was read as X (northing). "
                "If it is Y, swap the numbers.");
            return asXY;
        }
        if (asXY) return asXY;
        if (asYX) {
            asYX->note = appTr("Перше число прочитано як Y (схід), друге як X (північ).",
                               "The first number was read as Y (easting), the second as X (northing).");
            return asYX;
        }
        return std::nullopt;
    };

    auto mD = reDashed.match(s);
    if (mD.hasMatch()) {
        const double first  = (mD.captured(1) + mD.captured(2)).toLongLong();
        const double second = (mD.captured(3) + mD.captured(4)).toLongLong();
        if (auto r = tryEitherOrder(first, second)) return r;
    }

    // ── Labeled: X= ... Y= ...  or  X: ... Y: ... (any order) ────────────────
    static const QRegularExpression reXY(
        R"((?i)X\s*[=:]\s*([\d]+(?:[.,]\d+)?)\s*[;,\s]+\s*Y\s*[=:]\s*([\d]+(?:[.,]\d+)?))"
    );
    static const QRegularExpression reYX(
        R"((?i)Y\s*[=:]\s*([\d]+(?:[.,]\d+)?)\s*[;,\s]+\s*X\s*[=:]\s*([\d]+(?:[.,]\d+)?))"
    );

    auto mXY = reXY.match(s);
    if (mXY.hasMatch()) {
        if (auto r = tryXY(extractDouble(mXY.captured(1)), extractDouble(mXY.captured(2))))
            return r;
    }

    auto mYX = reYX.match(s);
    if (mYX.hasMatch()) {
        if (auto r = tryXY(extractDouble(mYX.captured(2)), extractDouble(mYX.captured(1))))
            return r;
    }

    // ── Unlabeled: two 6-8 digit numbers separated by comma, semicolon or space ─
    // Both must be 6+ digits (> 100 000) to avoid collision with DD/DMS.
    // Convention: first = X (northing), second = Y (easting with zone prefix).
    static const QRegularExpression reUnlabeled(
        R"(^(\d{6,8}(?:[.,]\d+)?)\s*[;,\s]+\s*(\d{6,8}(?:[.,]\d+)?)$)"
    );
    auto mU = reUnlabeled.match(s);
    if (mU.hasMatch()) {
        const double a = extractDouble(mU.captured(1));
        const double b = extractDouble(mU.captured(2));
        if (auto r = tryEitherOrder(a, b)) return r;
    }

    return std::nullopt;
}

QString CoordinateParser::formatName(CoordinateFormat format) {
    switch (format) {
        case CoordinateFormat::MGRS:    return "MGRS (Military Grid Reference System)";
        case CoordinateFormat::DD:      return "DD (Decimal Degrees)";
        case CoordinateFormat::DMS:     return "DMS (Degrees Minutes Seconds)";
        case CoordinateFormat::UTM:     return "UTM (Universal Transverse Mercator)";
        case CoordinateFormat::USK2000: return "УСК-2000 (Gauss-Kruger, k₀=1.0)";
        default:                        return "Невідомий";
    }
}
