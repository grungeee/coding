#include "GeoMath.h"

#include <math.h>

namespace {
constexpr double PI_D = 3.14159265358979323846;
constexpr double WGS84_A_M = 6378137.0;
constexpr double WGS84_FLATTENING = 1.0 / 298.257223563;
constexpr double WGS84_E2 = WGS84_FLATTENING * (2.0 - WGS84_FLATTENING);

double toRadians(double degrees) {
  return degrees * PI_D / 180.0;
}

double toDegrees(double radians) {
  return radians * 180.0 / PI_D;
}

void geodeticToEcef(const GeoPoint &point, double altitudeM, double &x, double &y, double &z) {
  const double lat = toRadians(point.latDeg);
  const double lon = toRadians(point.lonDeg);
  const double sinLat = sin(lat);
  const double cosLat = cos(lat);
  const double radius = WGS84_A_M / sqrt(1.0 - WGS84_E2 * sinLat * sinLat);

  x = (radius + altitudeM) * cosLat * cos(lon);
  y = (radius + altitudeM) * cosLat * sin(lon);
  z = (radius * (1.0 - WGS84_E2) + altitudeM) * sinLat;
}
} // namespace

void EnuFrame::setOrigin(const GeoPoint &origin, double altitudeM) {
  originLatRad_ = toRadians(origin.latDeg);
  originLonRad_ = toRadians(origin.lonDeg);
  geodeticToEcef(origin, altitudeM, originX_, originY_, originZ_);
  valid_ = true;
}

EnuPoint EnuFrame::toEnu(const GeoPoint &point, double altitudeM) const {
  if (!valid_) {
    return EnuPoint{0.0, 0.0, 0.0};
  }

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  geodeticToEcef(point, altitudeM, x, y, z);
  const double dx = x - originX_;
  const double dy = y - originY_;
  const double dz = z - originZ_;
  const double sinLat = sin(originLatRad_);
  const double cosLat = cos(originLatRad_);
  const double sinLon = sin(originLonRad_);
  const double cosLon = cos(originLonRad_);

  return EnuPoint{
      -sinLon * dx + cosLon * dy,
      -sinLat * cosLon * dx - sinLat * sinLon * dy + cosLat * dz,
      cosLat * cosLon * dx + cosLat * sinLon * dy + sinLat * dz,
  };
}

double normalizeDegrees(double degrees) {
  while (degrees < 0.0) {
    degrees += 360.0;
  }
  while (degrees >= 360.0) {
    degrees -= 360.0;
  }
  return degrees;
}

double shortestAngleDelta(double fromDeg, double toDeg) {
  double delta = normalizeDegrees(toDeg) - normalizeDegrees(fromDeg);
  if (delta > 180.0) {
    delta -= 360.0;
  } else if (delta < -180.0) {
    delta += 360.0;
  }
  return delta;
}

double enuDistanceMeters(const EnuPoint &from, const EnuPoint &to) {
  return hypot(to.eastM - from.eastM, to.northM - from.northM);
}

double enuBearingDegrees(const EnuPoint &from, const EnuPoint &to) {
  return normalizeDegrees(toDegrees(atan2(to.eastM - from.eastM, to.northM - from.northM)));
}

uint8_t ledIndexForRelativeBearing(double relativeBearingDeg, uint8_t ledCount) {
  if (ledCount == 0) {
    return 0;
  }
  const double step = 360.0 / static_cast<double>(ledCount);
  return static_cast<uint8_t>(floor((normalizeDegrees(relativeBearingDeg) + (step / 2.0)) / step)) % ledCount;
}
