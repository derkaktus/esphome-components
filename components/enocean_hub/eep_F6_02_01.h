#pragma once

#include "eep_base.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace enocean_hub {

/**
 * EEP F6-02-01  –  Rocker Switch, 2 Rocker
 *
 * RORG: F6 (RPS telegram, 1 data byte)
 *
 * Data byte layout:
 *   Bits 7-5 : Button identifier (R1)
 *     000 = AI  (rocker 1 top)
 *     001 = AO  (rocker 1 bottom)
 *     010 = BI  (rocker 2 top)
 *     011 = BO  (rocker 2 bottom)
 *     100 = CI  (3-rocker variant, unused here)
 *     …
 *   Bit  4   : Energy-bow (EB), 1 = button pressed, 0 = released
 *   Bits 3-1 : Button R2 (for simultaneous press; not decoded here)
 *   Bit  0   : T21 flag (always 1 for RPS)
 *
 * Exposed sensors:
 *   button_a_top    (binary) – AI pressed
 *   button_a_bottom (binary) – AO pressed
 *   button_b_top    (binary) – BI pressed
 *   button_b_bottom (binary) – BO pressed
 *   action          (text)   – human-readable last action
 */
class EepF6_02_01 : public EepBase {
 public:
  const char *eep_string() const override { return "F6-02-01"; }

  void set_button_a_top(binary_sensor::BinarySensor *s)    { btn_a_top_    = s; }
  void set_button_a_bottom(binary_sensor::BinarySensor *s) { btn_a_bottom_ = s; }
  void set_button_b_top(binary_sensor::BinarySensor *s)    { btn_b_top_    = s; }
  void set_button_b_bottom(binary_sensor::BinarySensor *s) { btn_b_bottom_ = s; }
  void set_action_sensor(text_sensor::TextSensor *s)       { action_       = s; }

  void parse(const EnoceanTelegram &tg) override;

 protected:
  binary_sensor::BinarySensor *btn_a_top_{nullptr};
  binary_sensor::BinarySensor *btn_a_bottom_{nullptr};
  binary_sensor::BinarySensor *btn_b_top_{nullptr};
  binary_sensor::BinarySensor *btn_b_bottom_{nullptr};
  text_sensor::TextSensor     *action_{nullptr};
};

}  // namespace enocean_hub
}  // namespace esphome
