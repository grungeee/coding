#include "PositionStability.h"

#include <math.h>

namespace {
constexpr double PI_D = 3.14159265358979323846;

double clampDouble(double value, double lower, double upper) {
  return value < lower ? lower : (value > upper ? upper : value);
}

EnuPoint addScaled(const EnuPoint &base, const EnuPoint &offset, double scale) {
  return EnuPoint{
      base.eastM + offset.eastM * scale,
      base.northM + offset.northM * scale,
      base.upM + offset.upM * scale,
  };
}
} // namespace

void AlphaBetaPositionFilter::reset(const EnuPoint &position, uint32_t nowMs) {
  valid_ = true;
  position_ = position;
  velocity_ = EnuPoint{0.0, 0.0, 0.0};
  lastUpdateMs_ = nowMs;
}

void AlphaBetaPositionFilter::update(const EnuPoint &measurement,
                                     uint32_t nowMs,
                                     const PositionFilterConfig &config) {
  if (!valid_) {
    reset(measurement, nowMs);
    return;
  }

  const double rawDt = static_cast<double>(nowMs - lastUpdateMs_) / 1000.0;
  if (rawDt <= 0.0 || rawDt > config.maxDtSeconds) {
    reset(measurement, nowMs);
    return;
  }

  const double alpha = clampDouble(config.alpha, 0.0, 1.0);
  const double beta = clampDouble(config.beta, 0.0, 1.0);
  const EnuPoint predicted = addScaled(position_, velocity_, rawDt);
  const EnuPoint residual{
      measurement.eastM - predicted.eastM,
      measurement.northM - predicted.northM,
      measurement.upM - predicted.upM,
  };

  position_ = addScaled(predicted, residual, alpha);
  velocity_ = addScaled(velocity_, residual, beta / rawDt);
  lastUpdateMs_ = nowMs;
}

void CircularAngleFilter::reset(double degrees) {
  const double radians = normalizeDegrees(degrees) * PI_D / 180.0;
  sin_ = sin(radians);
  cos_ = cos(radians);
  valid_ = true;
}

double CircularAngleFilter::update(double degrees, double alpha) {
  if (!valid_) {
    reset(degrees);
    return value();
  }

  const double radians = normalizeDegrees(degrees) * PI_D / 180.0;
  const double weight = clampDouble(alpha, 0.0, 1.0);
  sin_ = sin_ * (1.0 - weight) + sin(radians) * weight;
  cos_ = cos_ * (1.0 - weight) + cos(radians) * weight;
  const double magnitude = hypot(sin_, cos_);
  if (magnitude > 0.000001) {
    sin_ /= magnitude;
    cos_ /= magnitude;
  } else {
    reset(degrees);
  }
  return value();
}

double CircularAngleFilter::value() const {
  return normalizeDegrees(atan2(sin_, cos_) * 180.0 / PI_D);
}

uint8_t ledIndexWithHysteresis(double relativeBearingDeg,
                               uint8_t previousLed,
                               bool previousLedValid,
                               uint8_t ledCount,
                               double hysteresisDegrees) {
  if (!previousLedValid || ledCount == 0) {
    return ledIndexForRelativeBearing(relativeBearingDeg, ledCount);
  }

  const double step = 360.0 / static_cast<double>(ledCount);
  const double previousCenter = static_cast<double>(previousLed % ledCount) * step;
  const double boundary = step / 2.0 + (hysteresisDegrees > 0.0 ? hysteresisDegrees : 0.0);
  if (fabs(shortestAngleDelta(previousCenter, relativeBearingDeg)) <= boundary) {
    return previousLed % ledCount;
  }
  return ledIndexForRelativeBearing(relativeBearingDeg, ledCount);
}
