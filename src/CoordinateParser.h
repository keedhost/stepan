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
#include "CoordinateData.h"
#include <QString>
#include <optional>

class CoordinateParser {
public:
    static std::optional<CoordinateData> parse(const QString& input);
    static QString formatName(CoordinateFormat format);

private:
    static std::optional<CoordinateData> tryMGRS(const QString& input);
    static std::optional<CoordinateData> tryUTM(const QString& input);
    static std::optional<CoordinateData> tryDMS(const QString& input);
    static std::optional<CoordinateData> tryDD(const QString& input);
    static std::optional<CoordinateData> tryUSK2000(const QString& input);

    static CoordinateData buildFromLatLon(double lat, double lon, CoordinateFormat format);

    // USK-2000 helpers
    static void   latLonToUSK2000(double lat, double lon,
                                   double& X, double& Y, int& zone);
    static bool   usk2000ToLatLon(double X, double Y,
                                   double& lat, double& lon);
};
