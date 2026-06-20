#include "eep_F6_02_01.h"
#include "esphome/core/log.h"

namespace esphome {
namespace enocean_hub {

static const char *const TAG = "eep_F6_02_01";

// RORG for RPS telegrams
static const uint8_t RORG_RPS = 0xF6;

void EepF6_02_01::parse(const EnoceanTelegram &tg) {
  if (tg.rorg != RORG_RPS) {
    ESP_LOGW(TAG, "Unexpected RORG 0x%02X for F6-02-01 (expected 0xF6)", tg.rorg);
    return;
  }
  if (tg.data_len < 1) {
    ESP_LOGW(TAG, "F6-02-01: no data byte");
    return;
  }

  uint8_t db0 = tg.data[0];

  // Energy-bow bit: 1 = pressed, 0 = released
  bool pressed = (db0 & 0x10) != 0;

  // Button identifier bits [7:5]
  uint8_t btn_id = (db0 >> 5) & 0x07;

  // All buttons default to released each telegram; only the active one is set
  bool a_top    = false;
  bool a_bottom = false;
  bool b_top    = false;
  bool b_bottom = false;

  const char *label = "unknown";

  if (pressed) {
    switch (btn_id) {
      case 0b000: a_top    = true; label = "A_TOP pressed";    break;
      case 0b001: a_bottom = true; label = "A_BOTTOM pressed"; break;
      case 0b010: b_top    = true; label = "B_TOP pressed";    break;
      case 0b011: b_bottom = true; label = "B_BOTTOM pressed"; break;
      default:    label = "unknown button pressed";             break;
    }
  } else {
    label = "released";
  }

  ESP_LOGD(TAG, "F6-02-01 [0x%08X] DB0=0x%02X → %s", tg.sender_id, db0, label);

  if (btn_a_top_    != nullptr) btn_a_top_->publish_state(a_top);
  if (btn_a_bottom_ != nullptr) btn_a_bottom_->publish_state(a_bottom);
  if (btn_b_top_    != nullptr) btn_b_top_->publish_state(b_top);
  if (btn_b_bottom_ != nullptr) btn_b_bottom_->publish_state(b_bottom);
  if (action_       != nullptr) action_->publish_state(label);
}

}  // namespace enocean_hub
}  // namespace esphome
