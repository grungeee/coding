#pragma once

#include <stdint.h>

struct GeoPoint {
  double latDeg;
  double lonDeg;
};

// A fixed WGS84 local tangent plane. Once established from the first local
// GNSS fix, every navigation calculation takes place in metres in this frame.
struct EnuPoint {
  double eastM;
  double northM;
  double upM;
};

class EnuFrame {
public:
  void setOrigin(const GeoPoint &origin, double altitudeM);
  bool isValid() const { return valid_; }
  EnuPoint toEnu(const GeoPoint &point, double altitudeM) const;

private:
  bool valid_ = false;
  double originLatRad_ = 0.0;
  double originLonRad_ = 0.0;
  double originX_ = 0.0;
  double originY_ = 0.0;
  double originZ_ = 0.0;
};

double normalizeDegrees(double degrees);
double shortestAngleDelta(double fromDeg, double toDeg);
double enuDistanceMeters(const EnuPoint &from, const EnuPoint &to);
double enuBearingDegrees(const EnuPoint &from, const EnuPoint &to);
uint8_t ledIndexForRelativeBearing(double relativeBearingDeg, uint8_t ledCount);
