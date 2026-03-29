
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <esp_system.h>
#include <math.h>

constexpr uint16_t LED_COUNT = 16;
constexpr uint8_t LED_RING_PIN_A = 4; //not yet populated also pin 5 has a witre on it
constexpr uint8_t LED_RING_PIN_B = 3;
constexpr uint8_t LED_BRIGHTNESS = 150;
constexpr uint16_t BASE_FRAME_DELAY_MS = 30;
constexpr uint16_t BLINKER_FRAME_DELAY_MS = 30;
constexpr uint16_t MIN_FRAME_DELAY_MS = 12;
constexpr float BASE_PHASE_SPEED = 0.35f;

constexpr uint8_t MPU_SDA_PIN = 9;
constexpr uint8_t MPU_SCL_PIN = 8;
constexpr uint8_t MPU_ADDRESS = 0x68;
constexpr uint8_t MPU_REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t MPU_REG_CONFIG = 0x1A;
constexpr uint8_t MPU_REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU_REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t MPU_REG_ACCEL_XOUT_H = 0x3B;
constexpr uint8_t MPU_REG_GYRO_XOUT_H = 0x43;

constexpr float ACCEL_LSB_PER_G = 16384.0f;  // +/-2g
constexpr float GYRO_LSB_PER_DPS = 131.0f;   // +/-250 dps
constexpr float GYRO_SMOOTHING = 0.30f;
constexpr float MOTION_FACTOR_RISE_MAX_STEP = 0.045f;
constexpr float MOTION_FACTOR_FALL_MAX_STEP = 0.006f;
constexpr float MOTION_GYRO_SCALE = 300.0f;
constexpr float MOTION_GYRO_BIAS = 0.30f;
constexpr float MOTION_IDLE_DEADBAND = 0.10f;
constexpr uint32_t WARM_IDLE_DURATION_MS = 20000;
constexpr uint32_t EMBER_IDLE_FADE_DURATION_MS = 15000;
constexpr float STOP_TRIGGER_ENTER = 0.06f;
constexpr float STOP_TRIGGER_EXIT = 0.22f;
constexpr float STOP_CANCEL_THRESHOLD = 0.16f;
constexpr uint32_t STOP_BURST_DURATION_MS = 260;
constexpr uint32_t STOP_TOTAL_DURATION_MS = 3200;
constexpr uint32_t WARM_IDLE_START_DELAY_MS = 30000;
constexpr float WARM_IDLE_EXIT_THRESHOLD = 0.20f;
constexpr uint8_t BLINKER_LEFT_PIN = 6;
constexpr uint8_t BLINKER_RIGHT_PIN = 7;
constexpr uint32_t BLINKER_HALF_PERIOD_MS = 500;
constexpr uint8_t BLINKER_SEGMENT_LENGTH = LED_COUNT / 2;
constexpr uint8_t LEFT_BLINKER_INDEXES[BLINKER_SEGMENT_LENGTH] = {11, 10, 9, 8, 7, 6, 5, 4};
constexpr uint8_t RIGHT_BLINKER_INDEXES[BLINKER_SEGMENT_LENGTH] = {12, 13, 14, 15, 0, 1, 2, 3};
constexpr float TWO_PI_F = 6.28318530718f;
constexpr float PEAK_SWIRL_THRESHOLD = 0.78f;
constexpr float PEAK_SWIRL_SPEED = 0.010f;
constexpr float PEAK_SWIRL_STRENGTH = 0.32f;
constexpr float DEG_TO_RAD_F = 0.01745329252f;
constexpr float ONE_G_MPS2 = 9.80665f;
constexpr float ORIENTATION_KP = 3.2f;
constexpr float LINEAR_ACCEL_SMOOTHING = 0.22f;
constexpr float LINEAR_ACCEL_DEADZONE_MPS2 = 0.20f;
constexpr float LINEAR_ACCEL_MAX_MPS2 = 3.2f;
constexpr float LINEAR_ACCEL_HYST_ON_MPS2 = 0.55f;
constexpr float LINEAR_ACCEL_HYST_OFF_MPS2 = 0.30f;
constexpr float LINEAR_ACCEL_RESPONSE_GAMMA = 1.4f;
constexpr float MOTION_ACCEL_WEIGHT = 1.10f;
constexpr float MOTION_GYRO_WEIGHT = 0.30f;
constexpr int8_t FORWARD_AXIS_SIGN = -1;  // flip to 1 if the IMU's +Y already points toward forward acceleration
constexpr float LINEAR_ACCEL_BRAKE_DECAY_GAIN = 0.55f;
constexpr float FIRE_IDLE_RED = 1.0f;
constexpr float FIRE_IDLE_GREEN = 0.05f;
constexpr float FIRE_IDLE_BLUE = 0.0f;
constexpr float EMBER_FINAL_RED = 0.16f;
constexpr float EMBER_FINAL_GREEN = 0.02f;
constexpr float EMBER_FINAL_BLUE = 0.48f;
constexpr uint8_t FIRE_TOGGLE_REQUIRED_OFFS = 3;  // right-blinker cancellations needed to toggle fire effect
constexpr uint32_t FIRE_TOGGLE_MAX_INTERVAL_MS = 1500;  // max gap between offs before the sequence resets

Adafruit_NeoPixel exhaustRingA(LED_COUNT, LED_RING_PIN_A, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel exhaustRingB(LED_COUNT, LED_RING_PIN_B, NEO_GRB + NEO_KHZ800);
bool gMpuReady = false;
bool gStopEffectActive = false;
uint32_t gStopEffectStartMs = 0;
float gPrevMotionFactor = 0.0f;
bool gWarmIdleActive = false;
bool gWarmIdlePending = false;
uint32_t gWarmIdleStartMs = 0;
bool gEmberIdleActive = false;
uint32_t gEmberIdleStartMs = 0;
bool gFireEffectEnabled = true;
struct BlinkerState {
  bool previouslyActive = false;
  bool blinkOn = false;
  bool directionUp = true;
  uint32_t stateStartMs = 0;
};
BlinkerState gBlinkerState;

struct OrientationState {
  float q0 = 1.0f;
  float q1 = 0.0f;
  float q2 = 0.0f;
  float q3 = 0.0f;
  uint32_t lastUpdateMs = 0;
  bool initialized = false;
};

OrientationState gOrientationState;

struct MotionSample {
  float accelX = 0.0f;
  float accelY = 0.0f;
  float accelZ = 0.0f;
  float gyroX = 0.0f;
  float gyroY = 0.0f;
  float gyroZ = 0.0f;
  float accelMagnitudeG = 0.0f;
  float gyroMagnitudeDps = 0.0f;
  bool valid = false;
};

struct FireToggleCommandState {
  uint8_t completedOffs = 0;
  uint32_t lastOffMs = 0;
};

FireToggleCommandState gFireToggleCommandState;

void resetFireToggleCommandState() {
  gFireToggleCommandState.completedOffs = 0;
  gFireToggleCommandState.lastOffMs = 0;
}

void setFireEffectEnabled(bool enabled) {
  if (gFireEffectEnabled == enabled) {
    return;
  }
  gFireEffectEnabled = enabled;
  if (enabled) {
    Serial.println("Fire animation re-enabled");
  } else {
    Serial.println("Fire animation disabled via blinkers");
    gStopEffectActive = false;
    gWarmIdleActive = false;
    gWarmIdlePending = false;
    gEmberIdleActive = false;
    exhaustRingA.clear();
    exhaustRingB.clear();
    exhaustRingA.show();
    exhaustRingB.show();
  }
}

// Track consecutive right-blinker cancellations (falling edges) to toggle the fire animation.
void handleFireToggleGesture(bool rightFalling, uint32_t now) {
  if (!rightFalling) {
    if (gFireToggleCommandState.lastOffMs != 0 &&
        (now - gFireToggleCommandState.lastOffMs) > FIRE_TOGGLE_MAX_INTERVAL_MS) {
      resetFireToggleCommandState();
    }
    return;
  }

  if (gFireToggleCommandState.lastOffMs == 0 ||
      (now - gFireToggleCommandState.lastOffMs) <= FIRE_TOGGLE_MAX_INTERVAL_MS) {
    gFireToggleCommandState.completedOffs++;
  } else {
    gFireToggleCommandState.completedOffs = 1;
  }

  gFireToggleCommandState.lastOffMs = now;

  if (gFireToggleCommandState.completedOffs >= FIRE_TOGGLE_REQUIRED_OFFS) {
    setFireEffectEnabled(!gFireEffectEnabled);
    resetFireToggleCommandState();
  }
}

float fastInvSqrt(float value) {
  return 1.0f / sqrtf(value);
}

float computeForwardAccelerationMps2(const MotionSample &sample, uint32_t nowMs) {
  if (!sample.valid) {
    return 0.0f;
  }

  if (!gOrientationState.initialized) {
    gOrientationState.q0 = 1.0f;
    gOrientationState.q1 = 0.0f;
    gOrientationState.q2 = 0.0f;
    gOrientationState.q3 = 0.0f;
    gOrientationState.lastUpdateMs = nowMs;
    gOrientationState.initialized = true;
  }

  float dtSeconds = static_cast<float>(nowMs - gOrientationState.lastUpdateMs) * 0.001f;
  if (dtSeconds <= 0.0f || dtSeconds > 0.20f) {
    dtSeconds = 0.01f;
  }
  gOrientationState.lastUpdateMs = nowMs;

  float gxRad = sample.gyroX * DEG_TO_RAD_F;
  float gyRad = sample.gyroY * DEG_TO_RAD_F;
  float gzRad = sample.gyroZ * DEG_TO_RAD_F;

  float ax = sample.accelX;
  float ay = sample.accelY;
  float az = sample.accelZ;

  float accelNormSq = (ax * ax) + (ay * ay) + (az * az);
  if (accelNormSq > 0.0001f) {
    float accelRecip = fastInvSqrt(accelNormSq);
    ax *= accelRecip;
    ay *= accelRecip;
    az *= accelRecip;

    float halfvx = (gOrientationState.q1 * gOrientationState.q3) - (gOrientationState.q0 * gOrientationState.q2);
    float halfvy = (gOrientationState.q0 * gOrientationState.q1) + (gOrientationState.q2 * gOrientationState.q3);
    float halfvz = 0.5f - (gOrientationState.q1 * gOrientationState.q1) - (gOrientationState.q2 * gOrientationState.q2);

    float halfex = (ay * halfvz) - (az * halfvy);
    float halfey = (az * halfvx) - (ax * halfvz);
    float halfez = (ax * halfvy) - (ay * halfvx);

    gxRad += ORIENTATION_KP * halfex;
    gyRad += ORIENTATION_KP * halfey;
    gzRad += ORIENTATION_KP * halfez;
  }

  float qDot0 = 0.5f * ((-gOrientationState.q1 * gxRad) - (gOrientationState.q2 * gyRad) - (gOrientationState.q3 * gzRad));
  float qDot1 = 0.5f * ((gOrientationState.q0 * gxRad) + (gOrientationState.q2 * gzRad) - (gOrientationState.q3 * gyRad));
  float qDot2 = 0.5f * ((gOrientationState.q0 * gyRad) - (gOrientationState.q1 * gzRad) + (gOrientationState.q3 * gxRad));
  float qDot3 = 0.5f * ((gOrientationState.q0 * gzRad) + (gOrientationState.q1 * gyRad) - (gOrientationState.q2 * gxRad));

  gOrientationState.q0 += qDot0 * dtSeconds;
  gOrientationState.q1 += qDot1 * dtSeconds;
  gOrientationState.q2 += qDot2 * dtSeconds;
  gOrientationState.q3 += qDot3 * dtSeconds;

  float recipNorm = fastInvSqrt((gOrientationState.q0 * gOrientationState.q0) +
                                (gOrientationState.q1 * gOrientationState.q1) +
                                (gOrientationState.q2 * gOrientationState.q2) +
                                (gOrientationState.q3 * gOrientationState.q3));
  gOrientationState.q0 *= recipNorm;
  gOrientationState.q1 *= recipNorm;
  gOrientationState.q2 *= recipNorm;
  gOrientationState.q3 *= recipNorm;

  float gravityX = 2.0f * ((gOrientationState.q1 * gOrientationState.q3) - (gOrientationState.q0 * gOrientationState.q2));
  float gravityY = 2.0f * ((gOrientationState.q0 * gOrientationState.q1) + (gOrientationState.q2 * gOrientationState.q3));
  float gravityZ = (gOrientationState.q0 * gOrientationState.q0) - (gOrientationState.q1 * gOrientationState.q1) -
                   (gOrientationState.q2 * gOrientationState.q2) + (gOrientationState.q3 * gOrientationState.q3);

  float linearX = sample.accelX - gravityX;
  float linearY = sample.accelY - gravityY;
  float linearZ = sample.accelZ - gravityZ;

  float metricMps2 = 0.0f;
  float forwardAccelG = linearY * static_cast<float>(FORWARD_AXIS_SIGN);
  return forwardAccelG * ONE_G_MPS2;
}

float easedSine(float theta) {
  return (sinf(theta) + 1.0f) * 0.5f;
}

bool writeByte(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readBytes(uint8_t reg, uint8_t count, uint8_t *buffer) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  uint8_t bytesRead = Wire.requestFrom(MPU_ADDRESS, count);
  if (bytesRead != count) {
    return false;
  }
  for (uint8_t i = 0; i < count; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}

bool initializeMpu() {
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  delay(10);
  if (!writeByte(MPU_REG_PWR_MGMT_1, 0x01)) {  // auto select best clock
    return false;
  }
  delay(10);
  if (!writeByte(MPU_REG_CONFIG, 0x03)) {  // DLPF ~44 Hz
    return false;
  }
  if (!writeByte(MPU_REG_GYRO_CONFIG, 0x00)) {  // +/-250 dps
    return false;
  }
  if (!writeByte(MPU_REG_ACCEL_CONFIG, 0x00)) {  // +/-2 g
    return false;
  }
  return true;
}

MotionSample readMotionSample() {
  MotionSample sample;
  uint8_t buffer[14];
  if (!readBytes(MPU_REG_ACCEL_XOUT_H, sizeof(buffer), buffer)) {
    return sample;
  }

  auto combine = [](uint8_t high, uint8_t low) -> int16_t {
    return static_cast<int16_t>((static_cast<uint16_t>(high) << 8) | low);
  };

  int16_t axRaw = combine(buffer[0], buffer[1]);
  int16_t ayRaw = combine(buffer[2], buffer[3]);
  int16_t azRaw = combine(buffer[4], buffer[5]);

  int16_t gxRaw = combine(buffer[8], buffer[9]);
  int16_t gyRaw = combine(buffer[10], buffer[11]);
  int16_t gzRaw = combine(buffer[12], buffer[13]);

  float ax = static_cast<float>(axRaw) / ACCEL_LSB_PER_G;
  float ay = static_cast<float>(ayRaw) / ACCEL_LSB_PER_G;
  float az = static_cast<float>(azRaw) / ACCEL_LSB_PER_G;

  float gx = static_cast<float>(gxRaw) / GYRO_LSB_PER_DPS;
  float gy = static_cast<float>(gyRaw) / GYRO_LSB_PER_DPS;
  float gz = static_cast<float>(gzRaw) / GYRO_LSB_PER_DPS;

  sample.accelX = ax;
  sample.accelY = ay;
  sample.accelZ = az;
  sample.gyroX = gx;
  sample.gyroY = gy;
  sample.gyroZ = gz;
  sample.accelMagnitudeG = sqrtf((ax * ax) + (ay * ay) + (az * az));
  sample.gyroMagnitudeDps = sqrtf((gx * gx) + (gy * gy) + (gz * gz));
  sample.valid = true;
  return sample;
}

uint32_t composeExhaustColor(Adafruit_NeoPixel &strip,
                             uint16_t index,
                             uint32_t timeMs,
                             uint8_t phaseOffset,
                             float motionFactor,
                             float prevMotionFactor) {
  // Base palettes: warm tones at idle, blending to a purple peak at high motion.
  const float warmR = 0.92f;
  const float warmG = 0.30f;
  const float warmB = 0.04f;
  const float coolR = 0.74f;
  const float coolG = 0.08f;
  const float coolB = 0.98f;

  float normalizedMotion = fminf(fmaxf(motionFactor, 0.0f), 1.0f);
  float normalizedPrev = fminf(fmaxf(prevMotionFactor, 0.0f), 1.0f);
  bool coolingPhase = normalizedMotion < normalizedPrev;

  float persistenceMotion = coolingPhase ? ((normalizedMotion * 0.6f) + (normalizedPrev * 0.4f)) : normalizedMotion;
  float shapedMotion = powf(persistenceMotion, 1.40f);

  float wave = easedSine((static_cast<float>(index + phaseOffset) * 0.45f) + (static_cast<float>(timeMs) * 0.0045f));
  float shimmer = easedSine((static_cast<float>(index) * 0.90f) - (static_cast<float>(timeMs) * 0.0060f));
  float blend = (wave * 0.6f) + (shimmer * 0.4f);

  float baseIntensity = 0.001f;
  float dynamicBoost = shapedMotion * 1.05f;
  float waveGain = 0.40f + (shapedMotion * 0.5f);
  float intensity = (baseIntensity + dynamicBoost) * (0.46f + (blend * waveGain));
  intensity = fminf(fmaxf(intensity, 0.01f), 1.0f);

  if (shapedMotion > PEAK_SWIRL_THRESHOLD) {
    float normalizedSwirl = (shapedMotion - PEAK_SWIRL_THRESHOLD) / (1.0f - PEAK_SWIRL_THRESHOLD);
    float swirlPhase = (static_cast<float>(timeMs) * PEAK_SWIRL_SPEED) + (static_cast<float>(phaseOffset) * 0.18f);
    float swirlAngle = (static_cast<float>(index) / static_cast<float>(LED_COUNT)) * TWO_PI_F;
    float swirl = sinf(swirlAngle + swirlPhase);
    float modulation = 1.0f + (swirl * PEAK_SWIRL_STRENGTH * normalizedSwirl);
    intensity = fminf(fmaxf(intensity * modulation, 0.02f), 1.0f);
  }

  // Keep low/mid motion warm; reserve the purple palette for near-peak motion only.
  float coolMix = powf(shapedMotion, 2.35f);
  float warmMix = 1.0f - coolMix;

  float redRatio = (warmMix * warmR) + (coolMix * coolR);
  float greenRatio = (warmMix * warmG) + (coolMix * coolG);
  float blueRatio = (warmMix * warmB) + (coolMix * coolB);

  const float idleBlendStart = 0.55f;
  if (normalizedMotion < idleBlendStart) {
    float t = fminf(1.0f, (idleBlendStart - normalizedMotion) / idleBlendStart);
    float idleInfluence = powf(t, 1.35f);
    redRatio = (redRatio * (1.0f - idleInfluence)) + (FIRE_IDLE_RED * idleInfluence);
    greenRatio = (greenRatio * (1.0f - idleInfluence)) + (FIRE_IDLE_GREEN * idleInfluence);
    blueRatio = (blueRatio * (1.0f - idleInfluence)) + (FIRE_IDLE_BLUE * idleInfluence);
  }

  float redOffset = coolingPhase ? 0.017f : 0.020f;
  float greenOffset = coolingPhase ? 0.005f : 0.007f;
  float blueOffset = coolingPhase ? 0.004f : 0.005f;

  float red = fminf((redRatio * intensity) + redOffset, 1.0f);
  float green = fminf((greenRatio * intensity) + greenOffset, 1.0f);
  float blueControl = coolingPhase ? powf(normalizedMotion, 1.20f) : persistenceMotion;
  float blueBase = (blueRatio * intensity) + blueOffset;
  float blueMultiplier = coolingPhase ? (1.0f + 0.26f * blueControl) : (1.0f + 0.34f * blueControl);
  float blue = fminf(blueBase * blueMultiplier, 1.0f);

  uint8_t redByte = static_cast<uint8_t>(constrain(static_cast<int>(red * 255.0f), 0, 255));
  uint8_t greenByte = static_cast<uint8_t>(constrain(static_cast<int>(green * 255.0f), 0, 255));
  uint8_t blueByte = static_cast<uint8_t>(constrain(static_cast<int>(blue * 255.0f), 0, 255));

  if (shapedMotion > 0.16f) {
    uint8_t sparkChance = random(0, coolingPhase ? 165 : 135);
    uint8_t threshold = static_cast<uint8_t>(shapedMotion * (coolingPhase ? 22.0f : 32.0f));
    if (sparkChance < threshold) {
      uint8_t redBoost = random(24, coolingPhase ? 54 : 66);
      uint8_t amberBoost = random(4, coolingPhase ? 12 : 16);
      redByte = constrain(redByte + redBoost, 0, 255);
      greenByte = constrain(greenByte + amberBoost, 0, 255);
      if (!coolingPhase && shapedMotion > 0.72f) {
        blueByte = constrain(blueByte + random(10, 24), 0, 255);
      }
    }
  }

  return strip.Color(redByte, greenByte, blueByte);
}

void renderExhaustFrame(Adafruit_NeoPixel &strip, uint32_t timeMs, uint8_t phaseOffset, float accelFactor, float prevAccelFactor) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, composeExhaustColor(strip, i, timeMs, phaseOffset, accelFactor, prevAccelFactor));
  }
}

void renderStopFlareFrame(Adafruit_NeoPixel &strip, uint32_t elapsedMs) {
  if (elapsedMs >= STOP_TOTAL_DURATION_MS) {
    strip.clear();
    return;
  }

  if (elapsedMs <= STOP_BURST_DURATION_MS) {
    uint32_t burstColor = strip.Color(255, 48, 6);
    for (uint16_t i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, burstColor);
    }
    return;
  }

  uint32_t afterBurst = elapsedMs - STOP_BURST_DURATION_MS;
  float progress = static_cast<float>(afterBurst) / static_cast<float>(STOP_TOTAL_DURATION_MS - STOP_BURST_DURATION_MS);
  progress = fminf(fmaxf(progress, 0.0f), 1.0f);
  float decay = 1.0f - progress;
  float bounce = fabsf(sinf(static_cast<float>(afterBurst) * 0.012f));

  const float bottomRed = 1.0f;
  const float bottomGreen = 0.36f;
  const float bottomBlue = 0.04f;
  const float topRed = 0.52f;
  const float topGreen = 0.16f;
  const float topBlue = 1.0f;

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    float angle = (TWO_PI_F * static_cast<float>(i)) / static_cast<float>(LED_COUNT);
    float vertical = (sinf(angle) + 1.0f) * 0.5f;
    float mix = powf(vertical, 0.72f);

    float redRatio = bottomRed + ((topRed - bottomRed) * mix);
    float greenRatio = bottomGreen + ((topGreen - bottomGreen) * mix);
    float blueRatio = bottomBlue + ((topBlue - bottomBlue) * mix);

    float baseIntensity = (0.32f + (vertical * 0.85f)) * decay;
    if (vertical > 0.7f) {
      float bounceBoost = 0.62f * bounce;
      baseIntensity *= (0.7f + bounceBoost);
    }
    if (vertical < 0.28f) {
      float flickerBoost = 1.0f + (static_cast<float>(random(0, 16)) / 100.0f);
      baseIntensity *= flickerBoost;
    }

    float red = fminf(redRatio * baseIntensity, 1.0f);
    float green = fminf(greenRatio * baseIntensity, 1.0f);
    float blue = fminf(blueRatio * baseIntensity, 1.0f);

    if (vertical > 0.8f && random(0, 100) < 12) {
      float coolKick = 0.12f * (1.0f - progress);
      red = fminf(red + (coolKick * 0.28f), 1.0f);
      green = fminf(green + (coolKick * 0.18f), 1.0f);
      blue = fminf(blue + coolKick, 1.0f);
    }

    uint8_t redByte = static_cast<uint8_t>(constrain(static_cast<int>(red * 255.0f), 0, 255));
    uint8_t greenByte = static_cast<uint8_t>(constrain(static_cast<int>(green * 255.0f), 0, 255));
    uint8_t blueByte = static_cast<uint8_t>(constrain(static_cast<int>(blue * 255.0f), 0, 255));
    strip.setPixelColor(i, strip.Color(redByte, greenByte, blueByte));
  }
}

void renderWarmIdleFrame(Adafruit_NeoPixel &strip, uint32_t timeMs) {
  // Static idle: uniform dim red with subtle breathing to keep life without shifting hue.
  float baseIntensity = 0.18f + (0.02f * easedSine(static_cast<float>(timeMs) * 0.0022f));
  float microPulse = 0.015f * easedSine((static_cast<float>(timeMs) * 0.0051f) + 1.3f);

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    float angle = (TWO_PI_F * static_cast<float>(i)) / static_cast<float>(LED_COUNT);
    float positionalLift = 0.01f * sinf(angle * 1.5f);
    float intensity = fminf(fmaxf(baseIntensity + microPulse + positionalLift, 0.05f), 0.28f);

    uint8_t redByte = static_cast<uint8_t>(constrain(static_cast<int>(FIRE_IDLE_RED * intensity * 255.0f), 0, 255));
    uint8_t greenByte = static_cast<uint8_t>(constrain(static_cast<int>(FIRE_IDLE_GREEN * intensity * 255.0f), 0, 255));
    uint8_t blueByte = static_cast<uint8_t>(constrain(static_cast<int>(FIRE_IDLE_BLUE * intensity * 255.0f), 0, 255));

    strip.setPixelColor(i, strip.Color(redByte, greenByte, blueByte));
  }
}

void renderEmberIdleFrame(Adafruit_NeoPixel &strip, uint32_t timeMs, uint32_t elapsedMs) {
  float progress = fminf(static_cast<float>(elapsedMs) / static_cast<float>(EMBER_IDLE_FADE_DURATION_MS), 1.0f);
  float warmth = fmaxf(0.0f, 1.0f - powf(progress, 1.35f));

  float baseRed = (FIRE_IDLE_RED * warmth) + (EMBER_FINAL_RED * (1.0f - warmth));
  float baseGreen = (FIRE_IDLE_GREEN * warmth) + (EMBER_FINAL_GREEN * (1.0f - warmth));
  float baseBlue = (FIRE_IDLE_BLUE * warmth) + (EMBER_FINAL_BLUE * (1.0f - warmth));

  float baseIntensity = (0.22f * warmth) + (0.04f * (1.0f - warmth));
  float flickerPhase = easedSine(static_cast<float>(timeMs) * 0.0042f) * (0.04f * warmth);
  float emberPulse = easedSine(static_cast<float>(timeMs) * 0.0085f + 1.2f) * (0.03f * warmth);

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    float angle = (TWO_PI_F * static_cast<float>(i)) / static_cast<float>(LED_COUNT);
    float localLift = 0.08f * warmth * sinf(angle * 2.0f + static_cast<float>(elapsedMs) * 0.0008f);
    float intensity = baseIntensity + flickerPhase + emberPulse + localLift;
    float minIntensity = 0.04f + (0.01f * warmth);
    float maxIntensity = 0.28f * warmth + 0.05f;
    intensity = fminf(fmaxf(intensity, minIntensity), maxIntensity);

    uint8_t redByte = static_cast<uint8_t>(constrain(static_cast<int>(fminf(baseRed * intensity, 1.0f) * 255.0f), 0, 255));
    uint8_t greenByte = static_cast<uint8_t>(constrain(static_cast<int>(fminf(baseGreen * intensity, 1.0f) * 255.0f), 0, 255));
    uint8_t blueByte = static_cast<uint8_t>(constrain(static_cast<int>(fminf(baseBlue * intensity, 1.0f) * 255.0f), 0, 255));

    strip.setPixelColor(i, strip.Color(redByte, greenByte, blueByte));
  }
}

void renderBlinkerSegment(Adafruit_NeoPixel &strip,
                          const uint8_t *indexes,
                          float directedProgress) {
  // Blinker palette controls: base* sets idle amber, highlight* sets the sweeping pulse tint.
  const float baseRed = 1.0f;
  const float baseGreen = 0.14f;
  const float baseBlue = 0.0f;
  const float highlightRed = 1.0f;
  const float highlightGreen = 0.45f;  // bright yellow accent without drifting toward green
  const float highlightBlue = 0.0f;
  const float highlightSpan = 0.35f;

  for (uint8_t i = 0; i < BLINKER_SEGMENT_LENGTH; i++) {
    float position = static_cast<float>(i) / static_cast<float>(BLINKER_SEGMENT_LENGTH - 1);
    float distance = fabsf(position - directedProgress);
    // Envelope lifts pixels nearest the sweep position to emulate sequential turn signal bars.
    float highlight = fmaxf(0.0f, 1.0f - (distance / highlightSpan));
    highlight = powf(highlight, 1.75f);

    float mixBase = 1.0f - highlight;
    float brightness = 0.55f + (0.45f * highlight);

    float red = ((baseRed * mixBase) + (highlightRed * highlight)) * brightness;
    float green = ((baseGreen * mixBase) + (highlightGreen * highlight)) * brightness;
    float blue = ((baseBlue * mixBase) + (highlightBlue * highlight)) * brightness;

    // Keep the highlight firmly amber/yellow by nudging red upward.
    red = fminf(red + (0.10f * highlight), 1.0f);

    uint8_t redByte = static_cast<uint8_t>(constrain(static_cast<int>(red * 255.0f), 0, 255));
    uint8_t greenByte = static_cast<uint8_t>(constrain(static_cast<int>(green * 255.0f), 0, 255));
    uint8_t blueByte = static_cast<uint8_t>(constrain(static_cast<int>(blue * 255.0f), 0, 255));

    strip.setPixelColor(indexes[i], strip.Color(redByte, greenByte, blueByte));
  }
}

void renderBlinkerFrame(Adafruit_NeoPixel &strip,
                        bool leftActive,
                        bool rightActive,
                        float phaseProgress,
                        bool directionUp) {
  strip.clear();
  if (!leftActive && !rightActive) {
    return;
  }

  float directedProgress = directionUp ? phaseProgress : (1.0f - phaseProgress);
  directedProgress = fminf(fmaxf(directedProgress, 0.0f), 1.0f);

  if (leftActive) {
    renderBlinkerSegment(strip, LEFT_BLINKER_INDEXES, directedProgress);
  }
  if (rightActive) {
    renderBlinkerSegment(strip, RIGHT_BLINKER_INDEXES, directedProgress);
  }
}

void beginRing(Adafruit_NeoPixel &strip) {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void setup() {
  pinMode(BLINKER_LEFT_PIN, INPUT_PULLDOWN);
  pinMode(BLINKER_RIGHT_PIN, INPUT_PULLDOWN);
  beginRing(exhaustRingA);
  beginRing(exhaustRingB);
  randomSeed(esp_random());
  Serial.begin(115200);
  gMpuReady = initializeMpu();
  if (!gMpuReady) {
    Serial.println("MPU initialization failed");
  } else {
    Serial.println("MPU ready");
  }
}

void loop() {
  static uint8_t phaseShift = 0;
  static float phaseAccumulator = 0.0f;
  static float filteredLinearAccelMps2 = 0.0f;
  static float filteredGyroDps = 0.0f;
  static bool linearAccelGate = false;

  MotionSample sample = readMotionSample();
  uint32_t now = millis();
  if (!sample.valid) {
    if (gMpuReady) {
      Serial.println("MPU read failed");
      gMpuReady = false;
    }
    sample.accelX = 0.0f;
    sample.accelY = 0.0f;
    sample.accelZ = 1.0f;
    sample.gyroX = 0.0f;
    sample.gyroY = 0.0f;
    sample.gyroZ = 0.0f;
    sample.accelMagnitudeG = 1.0f;
    sample.gyroMagnitudeDps = 0.0f;
  } else {
    if (!gMpuReady) {
      Serial.println("MPU communication restored");
      gMpuReady = true;
    }
  }

  if (sample.valid) {
    filteredGyroDps += GYRO_SMOOTHING * (sample.gyroMagnitudeDps - filteredGyroDps);
  } else {
    filteredGyroDps += GYRO_SMOOTHING * (0.0f - filteredGyroDps);
  }

  float forwardAccelMps2 = computeForwardAccelerationMps2(sample, now);
  float accelPositive = fmaxf(forwardAccelMps2, 0.0f);
  float brakeAccel = fmaxf(-forwardAccelMps2, 0.0f);
  filteredLinearAccelMps2 += LINEAR_ACCEL_SMOOTHING * (accelPositive - filteredLinearAccelMps2);
  if (brakeAccel > 0.0f) {
    filteredLinearAccelMps2 = fmaxf(filteredLinearAccelMps2 - (brakeAccel * LINEAR_ACCEL_BRAKE_DECAY_GAIN), 0.0f);
    linearAccelGate = false;
  }

  if (!linearAccelGate && filteredLinearAccelMps2 >= LINEAR_ACCEL_HYST_ON_MPS2) {
    linearAccelGate = true;
  } else if (linearAccelGate && filteredLinearAccelMps2 <= LINEAR_ACCEL_HYST_OFF_MPS2) {
    linearAccelGate = false;
  }

  float accelForMapping = filteredLinearAccelMps2;
  if (accelForMapping <= LINEAR_ACCEL_DEADZONE_MPS2) {
    accelForMapping = 0.0f;
  } else {
    accelForMapping -= LINEAR_ACCEL_DEADZONE_MPS2;
  }

  float accelRange = LINEAR_ACCEL_MAX_MPS2 - LINEAR_ACCEL_DEADZONE_MPS2;
  if (accelRange <= 0.0f) {
    accelRange = 1.0f;
  }
  accelForMapping = fminf(accelForMapping, accelRange);
  float normalizedLinearAccel = accelForMapping / accelRange;
  if (!linearAccelGate) {
    normalizedLinearAccel = 0.0f;
  }

  float shapedAccel = powf(normalizedLinearAccel, LINEAR_ACCEL_RESPONSE_GAMMA);
  float dynamicGyroFactor = constrain(fmaxf(filteredGyroDps - MOTION_GYRO_BIAS, 0.0f) / MOTION_GYRO_SCALE, 0.0f, 1.0f);
  float targetMotionFactor = fminf((shapedAccel * MOTION_ACCEL_WEIGHT) + (dynamicGyroFactor * MOTION_GYRO_WEIGHT), 1.0f);
  if (targetMotionFactor <= MOTION_IDLE_DEADBAND) {
    targetMotionFactor = 0.0f;
  } else {
    targetMotionFactor = fminf((targetMotionFactor - MOTION_IDLE_DEADBAND) / (1.0f - MOTION_IDLE_DEADBAND), 1.0f);
  }

  static float motionFactorFiltered = 0.0f;
  float delta = targetMotionFactor - motionFactorFiltered;
  float maxStep = delta > 0.0f ? MOTION_FACTOR_RISE_MAX_STEP : MOTION_FACTOR_FALL_MAX_STEP;
  if (delta > maxStep) {
    delta = maxStep;
  } else if (delta < -maxStep) {
    delta = -maxStep;
  }
  motionFactorFiltered = fminf(fmaxf(motionFactorFiltered + delta, 0.0f), 1.0f);
  float motionFactor = motionFactorFiltered;

  bool leftBlinkerActive = digitalRead(BLINKER_LEFT_PIN) == HIGH;
  bool rightBlinkerActive = digitalRead(BLINKER_RIGHT_PIN) == HIGH;
  bool blinkersActive = leftBlinkerActive || rightBlinkerActive;

  static bool prevRightBlinkerActive = false;
  bool rightFalling = !rightBlinkerActive && prevRightBlinkerActive;
  handleFireToggleGesture(rightFalling, now);
  prevRightBlinkerActive = rightBlinkerActive;

  if (blinkersActive) {
    // Override exhaust animation to drive the sweeping turn signal segments.
    if (!gBlinkerState.previouslyActive) {
      gBlinkerState.previouslyActive = true;
      gBlinkerState.blinkOn = true;
      gBlinkerState.directionUp = true;
      gBlinkerState.stateStartMs = now;
    } else if (now - gBlinkerState.stateStartMs >= BLINKER_HALF_PERIOD_MS) {
      gBlinkerState.stateStartMs = now;
      gBlinkerState.blinkOn = !gBlinkerState.blinkOn;
      if (gBlinkerState.blinkOn) {
        gBlinkerState.directionUp = !gBlinkerState.directionUp;
      }
    }

    if (gBlinkerState.blinkOn) {
      float phaseProgress = static_cast<float>(now - gBlinkerState.stateStartMs) / static_cast<float>(BLINKER_HALF_PERIOD_MS);
      renderBlinkerFrame(exhaustRingA, leftBlinkerActive, rightBlinkerActive, phaseProgress, gBlinkerState.directionUp);
      renderBlinkerFrame(exhaustRingB, leftBlinkerActive, rightBlinkerActive, phaseProgress, gBlinkerState.directionUp);
    } else {
      exhaustRingA.clear();
      exhaustRingB.clear();
    }

    exhaustRingA.show();
    exhaustRingB.show();
    gPrevMotionFactor = motionFactor;
    delay(BLINKER_FRAME_DELAY_MS);
    return;
  } else if (gBlinkerState.previouslyActive) {
    gBlinkerState.previouslyActive = false;
    gBlinkerState.blinkOn = false;
    gBlinkerState.directionUp = true;
    exhaustRingA.clear();
    exhaustRingB.clear();
    exhaustRingA.show();
    exhaustRingB.show();
  }

  if (!gFireEffectEnabled) {
    gPrevMotionFactor = motionFactor;
    delay(BASE_FRAME_DELAY_MS);
    return;
  }

  if (!gStopEffectActive && (gPrevMotionFactor > STOP_TRIGGER_EXIT) && (motionFactor < STOP_TRIGGER_ENTER)) {
    gStopEffectActive = true;
    gStopEffectStartMs = now;
  }

  if (gStopEffectActive) {
    if (motionFactor > STOP_CANCEL_THRESHOLD) {
      gStopEffectActive = false;
    } else {
      uint32_t elapsed = now - gStopEffectStartMs;
      renderStopFlareFrame(exhaustRingA, elapsed);
      renderStopFlareFrame(exhaustRingB, elapsed);
      exhaustRingA.show();
      exhaustRingB.show();
      if (elapsed >= STOP_TOTAL_DURATION_MS) {
        gStopEffectActive = false;
        gWarmIdlePending = true;
        gWarmIdleActive = false;
        gWarmIdleStartMs = now;
        gEmberIdleActive = false;
      }
      gPrevMotionFactor = motionFactor;
      delay(BASE_FRAME_DELAY_MS);
      return;
    }
  }

  if (gWarmIdlePending && !gWarmIdleActive) {
    if (motionFactor > WARM_IDLE_EXIT_THRESHOLD) {
      gWarmIdlePending = false;
      gEmberIdleActive = false;
    } else if ((now - gWarmIdleStartMs) >= WARM_IDLE_START_DELAY_MS) {
      gWarmIdleActive = true;
      gWarmIdlePending = false;
      gWarmIdleStartMs = now;
      gEmberIdleActive = false;
    }
  }

  if (gWarmIdleActive) {
    if (motionFactor > WARM_IDLE_EXIT_THRESHOLD) {
      gWarmIdleActive = false;
      gEmberIdleActive = false;
    } else {
      uint32_t elapsedWarm = now - gWarmIdleStartMs;
      if (elapsedWarm >= WARM_IDLE_DURATION_MS) {
        gWarmIdleActive = false;
        gEmberIdleActive = true;
        gEmberIdleStartMs = now;
      } else {
        renderWarmIdleFrame(exhaustRingA, now);
        renderWarmIdleFrame(exhaustRingB, now + 37);
        exhaustRingA.show();
        exhaustRingB.show();
        gPrevMotionFactor = motionFactor;
        delay(BASE_FRAME_DELAY_MS);
        return;
      }
    }
  }

  if (gEmberIdleActive) {
    if (motionFactor > WARM_IDLE_EXIT_THRESHOLD) {
      gEmberIdleActive = false;
    } else {
      uint32_t elapsedEmber = now - gEmberIdleStartMs;
      renderEmberIdleFrame(exhaustRingA, now, elapsedEmber);
      renderEmberIdleFrame(exhaustRingB, now + 41, elapsedEmber);
      exhaustRingA.show();
      exhaustRingB.show();
      gPrevMotionFactor = motionFactor;
      delay(BASE_FRAME_DELAY_MS);
      return;
    }
  }

  float phaseStepFloat = BASE_PHASE_SPEED + (dynamicGyroFactor * 4.0f);
  phaseAccumulator += phaseStepFloat;
  if (phaseAccumulator >= 1.0f) {
    uint8_t steps = static_cast<uint8_t>(phaseAccumulator);
    phaseShift = (phaseShift + steps) % LED_COUNT;
    phaseAccumulator -= static_cast<float>(steps);
  }

  uint8_t opposingPhase = (phaseShift + (LED_COUNT / 2)) % LED_COUNT;

  renderExhaustFrame(exhaustRingA, now, phaseShift, motionFactor, gPrevMotionFactor);
  renderExhaustFrame(exhaustRingB, now, opposingPhase, motionFactor, gPrevMotionFactor);

  exhaustRingA.show();
  exhaustRingB.show();

  uint16_t dynamicDelay = BASE_FRAME_DELAY_MS - static_cast<uint16_t>(dynamicGyroFactor * 20.0f);
  if (dynamicDelay < MIN_FRAME_DELAY_MS) {
    dynamicDelay = MIN_FRAME_DELAY_MS;
  }
  delay(dynamicDelay);
  gPrevMotionFactor = motionFactor;
}
