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
