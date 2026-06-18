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

    // MGRS precision and zone bounds (valid only when isZone() is true)
    int    mgrsPrecision = 5;            // 5=1m point, 4=10m, 3=100m, 2=1km, 1=10km, 0=100km
    double zoneSWLat = 0, zoneSWLon = 0; // south-west corner of MGRS square
    double zoneNELat = 0, zoneNELon = 0; // north-east corner of MGRS square

    bool isValid() const { return detectedFormat != CoordinateFormat::Unknown; }
    bool isZone()  const { return detectedFormat == CoordinateFormat::MGRS && mgrsPrecision < 5; }
};
