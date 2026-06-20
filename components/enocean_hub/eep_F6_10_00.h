#pragma once

#include "eep_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace enocean_hub {

/**
 * EEP F6-10-00  –  Mechanical Handle (window/door handle sensor)
 *
 * RORG: F6 (RPS telegram, 1 data byte)
 *
 * Data byte values:
 *   0xF0 = Handle UP   (window tilted / open at top)
 *   0xE0 = Handle DOWN (window closed, handle pointing down — some encodings)
 *   0xD0 = Handle DOWN (alternative encoding)
 *   0xC0 = Handle HORIZONTAL (window fully open / handle horizontal)
 *
 * Note: actual byte values vary by manufacturer.
 * The three canonical positions are: closed, tilted, open.
 *
 * Exposed sensors:
 *   position      (sensor, float 0–2) – 0=closed, 1=tilted, 2=open
 *   position_text (text)              – "closed" | "tilted" | "open" | "unknown"
 */
class EepF6_10_00 : public EepBase {
 public:
  const char *eep_string() const override { return "F6-10-00"; }

  void set_position_sensor(sensor::Sensor *s)           { position_      = s; }
  void set_position_text_sensor(text_sensor::TextSensor *s) { position_text_ = s; }

  void parse(const EnoceanTelegram &tg) override;

 protected:
  sensor::Sensor          *position_{nullptr};
  text_sensor::TextSensor *position_text_{nullptr};
};

}  // namespace enocean_hub
}  // namespace esphome
