#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <vector>
#include <map>
#include <functional>

namespace esphome {
namespace enocean_hub {

// EnOcean UART telegram structure (ESP3 protocol)
// Sync Byte | Header (4B) | CRC8H | Data | Optional | CRC8D
static const uint8_t ENOCEAN_SYNC_BYTE = 0x55;
static const uint8_t PACKET_TYPE_RADIO_ERP1 = 0x01;

struct EnoceanTelegram {
  uint8_t rorg;           // Radio ORG (telegram type)
  uint8_t data[16];       // Payload bytes (without RORG and sender ID)
  uint8_t data_len;       // Length of payload
  uint32_t sender_id;     // 4-byte sender ID
  uint8_t status;         // Status byte
  int8_t rssi;            // Signal strength (optional data)
};

// Callback type: called when a telegram for a registered sender_id arrives
using EnoceanCallback = std::function<void(const EnoceanTelegram &)>;

// ─────────────────────────────────────────────────────────────────────────────
// EnoceanHub
// ─────────────────────────────────────────────────────────────────────────────
class EnoceanHub : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  /// Register a device callback for a specific sender_id + EEP string (e.g. "F6-02-01")
  void register_device(uint32_t sender_id, const std::string &eep,
                       EnoceanCallback callback);

  /// Enable debug_dev mode: every received telegram is forwarded to this text sensor
  void set_debug_sensor(text_sensor::TextSensor *sensor) {
    debug_sensor_ = sensor;
  }

 protected:
  // ── ESP3 parser state machine ──────────────────────────────────────────────
  enum class ParseState {
    WAIT_SYNC,
    READ_HEADER,
    READ_CRC_HEADER,
    READ_DATA,
    READ_CRC_DATA,
  };

  ParseState parse_state_{ParseState::WAIT_SYNC};
  uint8_t header_buf_[4]{};
  uint8_t header_pos_{0};
  uint16_t data_len_{0};
  uint8_t opt_len_{0};
  uint8_t packet_type_{0};
  std::vector<uint8_t> data_buf_;
  uint16_t data_pos_{0};

  void process_packet_();
  bool verify_crc8_(const uint8_t *data, size_t len, uint8_t expected);

  // ── Device registry ───────────────────────────────────────────────────────
  struct DeviceEntry {
    std::string eep;
    EnoceanCallback callback;
  };
  std::map<uint32_t, std::vector<DeviceEntry>> devices_;

  // ── Debug ─────────────────────────────────────────────────────────────────
  text_sensor::TextSensor *debug_sensor_{nullptr};

  // ── Helpers ───────────────────────────────────────────────────────────────
  static uint8_t crc8_(const uint8_t *data, size_t len);
  static std::string bytes_to_hex_(const uint8_t *data, size_t len);
};

}  // namespace enocean_hub
}  // namespace esphome
