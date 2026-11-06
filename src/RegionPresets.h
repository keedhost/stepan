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
#include <QVector>

// A named lat/lon bounding box a user can pick as a download region.
struct SubRegion {
    QString name;
    double south, west, north, east;
};

// A country/top-level entry in the region picker. subRegions[0] is always
// the "whole territory" entry; the rest are its administrative subdivisions.
struct CountryPreset {
    QString name;
    QVector<SubRegion> subRegions;
};

namespace RegionPresets {
// Bounding boxes sourced from OpenStreetMap / Nominatim administrative
// boundary data (2026), rounded to 4 decimal places.
const QVector<CountryPreset>& all();
}
