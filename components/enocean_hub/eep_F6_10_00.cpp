#include "eep_F6_10_00.h"
#include "esphome/core/log.h"

namespace esphome {
namespace enocean_hub {

static const char *const TAG = "eep_F6_10_00";

static const uint8_t RORG_RPS = 0xF6;

// Known handle position byte values
// Some manufacturers use the lower nibble, others only the upper nibble.
static const uint8_t HANDLE_CLOSED   = 0xF0;  // handle down (closed)
static const uint8_t HANDLE_TILTED   = 0xD0;  // handle up (tilted open)
static const uint8_t HANDLE_OPEN     = 0xC0;  // handle horizontal (fully open)
// Alternative closed byte used by some senders
static const uint8_t HANDLE_CLOSED_ALT = 0xE0;

void EepF6_10_00::parse(const EnoceanTelegram &tg) {
  if (tg.rorg != RORG_RPS) {
    ESP_LOGW(TAG, "Unexpected RORG 0x%02X for F6-10-00 (expected 0xF6)", tg.rorg);
    return;
  }
  if (tg.data_len < 1) {
    ESP_LOGW(TAG, "F6-10-00: no data byte");
    return;
  }

  uint8_t db0 = tg.data[0];
  float   position_val  = -1.0f;
  const char *position_str = "unknown";

  switch (db0) {
    case HANDLE_CLOSED:
    case HANDLE_CLOSED_ALT:
      position_val = 0.0f;
      position_str = "closed";
      break;
    case HANDLE_TILTED:
      position_val = 1.0f;
      position_str = "tilted";
      break;
    case HANDLE_OPEN:
      position_val = 2.0f;
      position_str = "open";
      break;
    default:
      ESP_LOGW(TAG, "F6-10-00 [0x%08X] unknown handle byte 0x%02X", tg.sender_id, db0);
      break;
  }

  ESP_LOGD(TAG, "F6-10-00 [0x%08X] DB0=0x%02X → %s", tg.sender_id, db0, position_str);

  if (position_ != nullptr && position_val >= 0.0f)
    position_->publish_state(position_val);
  if (position_text_ != nullptr)
    position_text_->publish_state(position_str);
}

}  // namespace enocean_hub
}  // namespace esphome
