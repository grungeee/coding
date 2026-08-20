#include <assert.h>

#include "../GeoMath.h"
#include "../PositionStability.h"

int main() {
  EnuFrame frame;
  frame.setOrigin(GeoPoint{48.2082, 16.3738}, 170.0);
  const EnuPoint east = frame.toEnu(GeoPoint{48.2082, 16.3739}, 170.0);
  assert(east.eastM > 5.0 && east.eastM < 10.0);
  assert(enuBearingDegrees(EnuPoint{0.0, 0.0, 0.0}, east) > 80.0);

  const PositionFilterConfig filterConfig{0.5, 0.1, 10.0};
  AlphaBetaPositionFilter filter;
  filter.reset(EnuPoint{0.0, 0.0, 0.0}, 0);
  filter.update(EnuPoint{10.0, 0.0, 0.0}, 1000, filterConfig);
  assert(filter.position().eastM > 4.9 && filter.position().eastM < 5.1);

  CircularAngleFilter angle;
  angle.reset(359.0);
  const double north = angle.update(1.0, 0.5);
  assert(__builtin_fabs(shortestAngleDelta(0.0, north)) < 2.0);

  const uint8_t initialLed = ledIndexWithHysteresis(0.0, 0, false, 24, 4.0);
  assert(initialLed == 0);
  assert(ledIndexWithHysteresis(8.0, initialLed, true, 24, 4.0) == 0);
  assert(ledIndexWithHysteresis(12.0, initialLed, true, 24, 4.0) == 1);
  return 0;
}
