#pragma once

#include <Arduino.h>

constexpr uint16_t MESH_MAGIC = 0x464D; // "MF" in little-endian memory.
constexpr uint8_t MESH_VERSION = 2;
constexpr uint8_t MESH_PACKET_POSITION = 1;
constexpr uint8_t MESH_DEFAULT_HOP_LIMIT = 3;

constexpr uint8_t FLAG_GNSS_VALID = 0x01;
constexpr uint8_t FLAG_COMPASS_VALID = 0x02;
constexpr uint8_t FLAG_LOW_BATTERY = 0x04;

struct __attribute__((packed)) MeshPacketV2 {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
  uint32_t nodeId;
  uint16_t sequence;
  uint32_t uptimeMs;
  int32_t latE7;
  int32_t lonE7;
  int16_t altM;
  uint16_t speedCentiKph;
  uint16_t courseCdeg;
  uint16_t headingCdeg;
  uint16_t hdopCenti;
  uint8_t sats;
  uint8_t flags;
  uint16_t batteryMv;
  uint8_t hopLimit;
  uint8_t hopCount;
  uint16_t crc;
};

static_assert(sizeof(MeshPacketV2) == 40, "Unexpected mesh packet size");

inline uint16_t crc16Ccitt(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

inline void finalizePacket(MeshPacketV2 &packet) {
  packet.magic = MESH_MAGIC;
  packet.version = MESH_VERSION;
  packet.type = MESH_PACKET_POSITION;
  packet.crc = 0;
  packet.crc = crc16Ccitt(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
}

inline bool packetIsValid(const MeshPacketV2 &packet) {
  if (packet.magic != MESH_MAGIC || packet.version != MESH_VERSION || packet.type != MESH_PACKET_POSITION) {
    return false;
  }

  if ((packet.flags & FLAG_GNSS_VALID) &&
      (packet.latE7 < -900000000 || packet.latE7 > 900000000 ||
       packet.lonE7 < -1800000000 || packet.lonE7 > 1800000000)) {
    return false;
  }

  MeshPacketV2 copy = packet;
  const uint16_t expected = copy.crc;
  copy.crc = 0;
  return crc16Ccitt(reinterpret_cast<const uint8_t *>(&copy), sizeof(copy)) == expected;
}
