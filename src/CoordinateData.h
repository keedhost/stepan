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
#include <QString>

enum class CoordinateFormat {
    Unknown,
    MGRS,
    DD,
    DMS,
    UTM,
    USK2000
};

struct CoordinateData {
    double lat = 0.0;
    double lon = 0.0;

    QString mgrs;
    QString dd;
    QString dms;
    QString utm;
    QString usk2000; // "X: 5487123.456; Y: 6312456.789 (зона 6)"

    CoordinateFormat detectedFormat = CoordinateFormat::Unknown;

    // Optional remark about how the input was read (e.g. an ambiguous X/Y
    // order in USK-2000). Empty when there is nothing to say.
    QString note;

    // MGRS precision and zone bounds (valid only when isZone() is true)
    int    mgrsPrecision = 5;            // 5=1m point, 4=10m, 3=100m, 2=1km, 1=10km, 0=100km
    double zoneSWLat = 0, zoneSWLon = 0; // south-west corner of MGRS square
    double zoneNELat = 0, zoneNELon = 0; // north-east corner of MGRS square

    bool isValid() const { return detectedFormat != CoordinateFormat::Unknown; }
    bool isZone()  const { return detectedFormat == CoordinateFormat::MGRS && mgrsPrecision < 5; }
};
