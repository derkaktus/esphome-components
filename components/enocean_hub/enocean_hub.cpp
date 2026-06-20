#include "enocean_hub.h"
#include "esphome/core/log.h"
#include <cstring>
#include <cstdio>

namespace esphome {
namespace enocean_hub {

static const char *const TAG = "enocean_hub";

// ─────────────────────────────────────────────────────────────────────────────
// CRC-8 lookup (polynomial 0x07, as used by EnOcean ESP3)
// ─────────────────────────────────────────────────────────────────────────────
static const uint8_t CRC8_TABLE[256] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31,
    0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
    0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9,
    0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1,
    0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
    0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe,
    0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16,
    0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
    0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
    0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8,
    0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
    0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10,
    0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f,
    0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
    0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7,
    0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
    0xfa, 0xfd, 0xf4, 0xf3,
};

uint8_t EnoceanHub::crc8_(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++)
    crc = CRC8_TABLE[crc ^ data[i]];
  return crc;
}

std::string EnoceanHub::bytes_to_hex_(const uint8_t *data, size_t len) {
  std::string out;
  out.reserve(len * 3);
  char tmp[4];
  for (size_t i = 0; i < len; i++) {
    snprintf(tmp, sizeof(tmp), "%02X ", data[i]);
    out += tmp;
  }
  if (!out.empty())
    out.pop_back();  // remove trailing space
  return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// setup / dump_config
// ─────────────────────────────────────────────────────────────────────────────
void EnoceanHub::setup() {
  ESP_LOGCONFIG(TAG, "EnOcean Hub starting up...");
}

void EnoceanHub::dump_config() {
  ESP_LOGCONFIG(TAG, "EnOcean Hub:");
  ESP_LOGCONFIG(TAG, "  Registered devices: %d", (int) devices_.size());
  if (debug_sensor_ != nullptr)
    ESP_LOGCONFIG(TAG, "  Debug sensor: enabled");
  this->check_uart_settings(57600);
}

// ─────────────────────────────────────────────────────────────────────────────
// Device registration
// ─────────────────────────────────────────────────────────────────────────────
void EnoceanHub::register_device(uint32_t sender_id, const std::string &eep,
                                 EnoceanCallback callback) {
  devices_[sender_id].push_back({eep, std::move(callback)});
  ESP_LOGD(TAG, "Registered device 0x%08X with EEP %s", sender_id, eep.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Main loop – ESP3 state machine
// ─────────────────────────────────────────────────────────────────────────────
void EnoceanHub::loop() {
  while (available()) {
    uint8_t byte = read();

    switch (parse_state_) {
      // ── Wait for sync byte 0x55 ─────────────────────────────────────────
      case ParseState::WAIT_SYNC:
        if (byte == ENOCEAN_SYNC_BYTE) {
          header_pos_ = 0;
          parse_state_ = ParseState::READ_HEADER;
        }
        break;

      // ── Read 4-byte header: DataLength(2) | OptLength(1) | PacketType(1) ──
      case ParseState::READ_HEADER:
        header_buf_[header_pos_++] = byte;
        if (header_pos_ == 4) {
          parse_state_ = ParseState::READ_CRC_HEADER;
        }
        break;

      // ── Verify header CRC ────────────────────────────────────────────────
      case ParseState::READ_CRC_HEADER: {
        uint8_t expected_crc = crc8_(header_buf_, 4);
        if (byte != expected_crc) {
          ESP_LOGW(TAG, "Header CRC mismatch (got 0x%02X expected 0x%02X)", byte, expected_crc);
          parse_state_ = ParseState::WAIT_SYNC;
          break;
        }
        data_len_    = ((uint16_t) header_buf_[0] << 8) | header_buf_[1];
        opt_len_     = header_buf_[2];
        packet_type_ = header_buf_[3];

        uint16_t total = data_len_ + opt_len_;
        if (total == 0 || total > 256) {
          ESP_LOGW(TAG, "Implausible packet length %d, discarding", total);
          parse_state_ = ParseState::WAIT_SYNC;
          break;
        }
        data_buf_.resize(total);
        data_pos_ = 0;
        parse_state_ = ParseState::READ_DATA;
        break;
      }

      // ── Read data + optional bytes ───────────────────────────────────────
      case ParseState::READ_DATA:
        data_buf_[data_pos_++] = byte;
        if (data_pos_ == (uint16_t)(data_len_ + opt_len_))
          parse_state_ = ParseState::READ_CRC_DATA;
        break;

      // ── Verify data CRC and dispatch ─────────────────────────────────────
      case ParseState::READ_CRC_DATA: {
        uint8_t expected_crc = crc8_(data_buf_.data(), data_buf_.size());
        if (byte != expected_crc) {
          ESP_LOGW(TAG, "Data CRC mismatch (got 0x%02X expected 0x%02X)", byte, expected_crc);
          parse_state_ = ParseState::WAIT_SYNC;
          break;
        }
        if (packet_type_ == PACKET_TYPE_RADIO_ERP1)
          process_packet_();
        parse_state_ = ParseState::WAIT_SYNC;
        break;
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// process_packet_ – extract telegram fields and dispatch to devices
// ─────────────────────────────────────────────────────────────────────────────
void EnoceanHub::process_packet_() {
  // ERP1 layout: RORG(1) | data(N) | SenderID(4) | Status(1)
  // data_len_ includes RORG + payload + SenderID + Status
  if (data_len_ < 6) {  // RORG(1) + SenderID(4) + Status(1)
    ESP_LOGW(TAG, "ERP1 packet too short (%d)", data_len_);
    return;
  }

  EnoceanTelegram tg;
  tg.rorg    = data_buf_[0];
  tg.data_len = (uint8_t)(data_len_ - 6);          // payload without RORG/ID/status
  if (tg.data_len > sizeof(tg.data)) {
    ESP_LOGW(TAG, "ERP1 payload too long (%d > %d), truncating", tg.data_len, (int) sizeof(tg.data));
    tg.data_len = sizeof(tg.data);
  }
  memcpy(tg.data, &data_buf_[1], tg.data_len);

  uint32_t id = ((uint32_t) data_buf_[data_len_ - 5] << 24) |
                ((uint32_t) data_buf_[data_len_ - 4] << 16) |
                ((uint32_t) data_buf_[data_len_ - 3] << 8)  |
                 (uint32_t) data_buf_[data_len_ - 2];
  tg.sender_id = id;
  tg.status    = data_buf_[data_len_ - 1];

  // RSSI is the last byte of the optional section (if present)
  tg.rssi = (opt_len_ > 0) ? -(int8_t) data_buf_[data_len_ + opt_len_ - 1] : 0;

  ESP_LOGD(TAG, "Telegram RORG=0x%02X ID=0x%08X RSSI=%d dBm", tg.rorg, tg.sender_id, tg.rssi);

  // ── debug_dev sensor: publish hex dump of every telegram ──────────────────
  if (debug_sensor_ != nullptr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "RORG:%02X ID:%08X DATA:%s RSSI:%ddBm",
             tg.rorg, tg.sender_id,
             bytes_to_hex_(tg.data, tg.data_len).c_str(),
             tg.rssi);
    debug_sensor_->publish_state(buf);
  }

  // ── dispatch to registered devices ────────────────────────────────────────
  auto it = devices_.find(tg.sender_id);
  if (it == devices_.end()) {
    ESP_LOGD(TAG, "No device registered for ID 0x%08X", tg.sender_id);
    return;
  }
  for (auto &entry : it->second)
    entry.callback(tg);
}

}  // namespace enocean_hub
}  // namespace esphome
