#pragma once

#include <Arduino.h>
#include "MeshPacket.h"
#include "PositionStability.h"
#include "config.h"

struct Peer {
  bool active = false;
  uint32_t nodeId = 0;
  uint16_t sequence = 0;
  uint32_t lastSeenMs = 0;
  int32_t latE7 = 0;
  int32_t lonE7 = 0;
  int16_t altM = 0;
  uint16_t headingCdeg = 0;
  uint16_t hdopCenti = 0;
  uint16_t batteryMv = 0;
  int16_t rssiDbm = 0;
  float snrDb = 0.0f;
  uint8_t sats = 0;
  uint8_t flags = 0;
  uint8_t hopCount = 0;
  uint32_t originUptimeMs = 0;
  bool originUptimeValid = false;
  AlphaBetaPositionFilter positionFilter;
  CircularAngleFilter bearingFilter;
  bool directionLedValid = false;
  uint8_t directionLed = 0;
  bool nearbyUncertain = false;
  uint32_t nearbySinceMs = 0;
  double heldBearingDeg = 0.0;
};

enum class PeerUpdateResult : uint8_t {
  Accepted,
  DuplicateOrOutOfOrder,
  Stale,
  UnrealisticJump,
};

class PeerTable {
public:
  PeerUpdateResult upsert(const MeshPacketV2 &packet,
                          int16_t rssiDbm,
                          float snrDb,
                          uint32_t nowMs,
                          const EnuFrame &navigationFrame,
                          const PositionFilterConfig &filterConfig);
  bool accepts(uint32_t nodeId, uint16_t sequence) const;
  void initializePositions(const EnuFrame &navigationFrame, uint32_t nowMs);
  void prune(uint32_t nowMs);
  size_t countActive(uint32_t nowMs) const;
  Peer *begin() { return peers_; }
  Peer *end() { return peers_ + MAX_PEERS; }
  const Peer *begin() const { return peers_; }
  const Peer *end() const { return peers_ + MAX_PEERS; }

private:
  Peer *findSlot(uint32_t nodeId, uint32_t nowMs);
  Peer peers_[MAX_PEERS];
};
