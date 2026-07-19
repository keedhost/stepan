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
