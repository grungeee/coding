#include "PeerTable.h"

namespace {
bool uptimeIsNewer(uint32_t candidate, uint32_t previous) {
  return static_cast<int32_t>(candidate - previous) > 0;
}

GeoPoint packetLocation(const MeshPacketV2 &packet) {
  return GeoPoint{
      static_cast<double>(packet.latE7) / 10000000.0,
      static_cast<double>(packet.lonE7) / 10000000.0,
  };
}

GeoPoint peerLocation(const Peer &peer) {
  return GeoPoint{
      static_cast<double>(peer.latE7) / 10000000.0,
      static_cast<double>(peer.lonE7) / 10000000.0,
  };
}

double packetElapsedSeconds(const Peer &peer, const MeshPacketV2 &packet, uint32_t nowMs) {
  if (peer.originUptimeValid && uptimeIsNewer(packet.uptimeMs, peer.originUptimeMs)) {
    return static_cast<double>(packet.uptimeMs - peer.originUptimeMs) / 1000.0;
  }
  return static_cast<double>(nowMs - peer.lastSeenMs) / 1000.0;
}
} // namespace

Peer *PeerTable::findSlot(uint32_t nodeId, uint32_t nowMs) {
  Peer *oldest = &peers_[0];
  for (Peer &peer : peers_) {
    if (peer.active && peer.nodeId == nodeId) {
      return &peer;
    }
    if (!peer.active) {
      return &peer;
    }
    if (nowMs - peer.lastSeenMs > nowMs - oldest->lastSeenMs) {
      oldest = &peer;
    }
  }

  for (Peer &peer : peers_) {
    if (nowMs - peer.lastSeenMs > PEER_TIMEOUT_MS) {
      return &peer;
    }
  }

  return oldest;
}

bool PeerTable::accepts(uint32_t nodeId, uint16_t sequence) const {
  for (const Peer &peer : peers_) {
    if (!peer.active || peer.nodeId != nodeId) {
      continue;
    }
    return static_cast<int16_t>(sequence - peer.sequence) > 0;
  }
  return true;
}

PeerUpdateResult PeerTable::upsert(const MeshPacketV2 &packet,
                                   int16_t rssiDbm,
                                   float snrDb,
                                   uint32_t nowMs,
                                   const EnuFrame &navigationFrame,
                                   const PositionFilterConfig &filterConfig) {
  Peer *slot = findSlot(packet.nodeId, nowMs);
  // A sender may reboot and restart its sequence/uptime counters. Once the
  // old table entry has expired, treat its next packet as a fresh peer.
  const bool existingPeer = slot->active && slot->nodeId == packet.nodeId &&
                            nowMs - slot->lastSeenMs <= PEER_TIMEOUT_MS;
  if (existingPeer && !accepts(packet.nodeId, packet.sequence)) {
    return PeerUpdateResult::DuplicateOrOutOfOrder;
  }
  if (existingPeer && slot->originUptimeValid && !uptimeIsNewer(packet.uptimeMs, slot->originUptimeMs)) {
    return PeerUpdateResult::Stale;
  }

  EnuPoint measurement = {0.0, 0.0, 0.0};
  const bool hasPosition = (packet.flags & FLAG_GNSS_VALID) != 0;
  if (hasPosition && navigationFrame.isValid()) {
    measurement = navigationFrame.toEnu(packetLocation(packet), packet.altM);
    if (existingPeer && slot->positionFilter.isValid()) {
      const double elapsedSeconds = packetElapsedSeconds(*slot, packet, nowMs);
      const double allowedJump = POSITION_JUMP_ALLOWANCE_M +
                                 POSITION_MAX_SPEED_MPS * (elapsedSeconds > 0.0 ? elapsedSeconds : 0.0);
      if (enuDistanceMeters(slot->positionFilter.position(), measurement) > allowedJump) {
        return PeerUpdateResult::UnrealisticJump;
      }
    }
  }

  if (!existingPeer) {
    *slot = Peer{};
  }
  slot->active = true;
  slot->nodeId = packet.nodeId;
  slot->sequence = packet.sequence;
  slot->lastSeenMs = nowMs;
  slot->originUptimeMs = packet.uptimeMs;
  slot->originUptimeValid = true;
  if (hasPosition) {
    slot->latE7 = packet.latE7;
    slot->lonE7 = packet.lonE7;
    slot->altM = packet.altM;
    if (navigationFrame.isValid()) {
      slot->positionFilter.update(measurement, nowMs, filterConfig);
    }
  }
  slot->headingCdeg = packet.headingCdeg;
  slot->hdopCenti = packet.hdopCenti;
  slot->batteryMv = packet.batteryMv;
  slot->rssiDbm = rssiDbm;
  slot->snrDb = snrDb;
  slot->sats = packet.sats;
  slot->flags = packet.flags;
  slot->hopCount = packet.hopCount;
  return PeerUpdateResult::Accepted;
}

void PeerTable::initializePositions(const EnuFrame &navigationFrame, uint32_t nowMs) {
  if (!navigationFrame.isValid()) {
    return;
  }

  for (Peer &peer : peers_) {
    if (!peer.active || !(peer.flags & FLAG_GNSS_VALID) || peer.positionFilter.isValid()) {
      continue;
    }
    peer.positionFilter.reset(navigationFrame.toEnu(peerLocation(peer), peer.altM), nowMs);
  }
}

void PeerTable::prune(uint32_t nowMs) {
  for (Peer &peer : peers_) {
    if (peer.active && nowMs - peer.lastSeenMs > PEER_TIMEOUT_MS) {
      peer.active = false;
    }
  }
}

size_t PeerTable::countActive(uint32_t nowMs) const {
  size_t count = 0;
  for (const Peer &peer : peers_) {
    if (peer.active && nowMs - peer.lastSeenMs <= PEER_TIMEOUT_MS) {
      count++;
    }
  }
  return count;
}
