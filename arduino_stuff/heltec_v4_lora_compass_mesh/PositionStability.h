#pragma once

#include <stdint.h>

#include "GeoMath.h"

struct PositionFilterConfig {
  double alpha;
  double beta;
  double maxDtSeconds;
};

// Constant-velocity alpha-beta filter in the local ENU frame.
class AlphaBetaPositionFilter {
public:
  bool isValid() const { return valid_; }
  void reset(const EnuPoint &position, uint32_t nowMs);
  void update(const EnuPoint &measurement, uint32_t nowMs, const PositionFilterConfig &config);
  EnuPoint position() const { return position_; }
  EnuPoint velocity() const { return velocity_; }

private:
  bool valid_ = false;
  EnuPoint position_ = {0.0, 0.0, 0.0};
  EnuPoint velocity_ = {0.0, 0.0, 0.0};
  uint32_t lastUpdateMs_ = 0;
};

// Filters angles as unit vectors, so updates crossing north remain continuous.
class CircularAngleFilter {
public:
  bool isValid() const { return valid_; }
  void reset(double degrees);
  double update(double degrees, double alpha);
  double value() const;

private:
  bool valid_ = false;
  double sin_ = 0.0;
  double cos_ = 1.0;
};

uint8_t ledIndexWithHysteresis(double relativeBearingDeg,
                               uint8_t previousLed,
                               bool previousLedValid,
                               uint8_t ledCount,
                               double hysteresisDegrees);
