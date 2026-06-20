#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "enocean_hub.h"
#include "eep_base.h"
#include "eep_F6_02_01.h"
#include "eep_F6_10_00.h"

#include <memory>

namespace esphome {
namespace enocean_hub {

/**
 * EnoceanDevice – a logical EnOcean device linked to the hub.
 *
 * In YAML this maps to:
 *
 *   binary_sensor:
 *     - platform: enocean_hub
 *       enocean_hub_id: my_enocean_hub
 *       sender_id: "0xDEADBEEF"
 *       eep: "F6-02-01"
 *       button_a_top:
 *         name: "Rocker A top"
 *       …
 *
 *   sensor:
 *     - platform: enocean_hub
 *       enocean_hub_id: my_enocean_hub
 *       sender_id: "0xAABBCCDD"
 *       eep: "F6-10-00"
 *       position:
 *         name: "Window position"
 *
 * The codegen_ layer (sensor.py / binary_sensor.py) wires up everything;
 * this class just holds the EEP parser and registers itself with the hub.
 */
class EnoceanDevice : public Component {
 public:
  void set_hub(EnoceanHub *hub)          { hub_ = hub; }
  void set_sender_id(uint32_t id)        { sender_id_ = id; }
  void set_eep(std::unique_ptr<EepBase> eep) { eep_ = std::move(eep); }

  void setup() override {
    hub_->register_device(sender_id_, eep_->eep_string(),
                          [this](const EnoceanTelegram &tg) { eep_->parse(tg); });
  }

  void dump_config() override;

  float get_setup_priority() const override {
    // Must run after the hub
    return setup_priority::DATA - 1.0f;
  }

 protected:
  EnoceanHub             *hub_{nullptr};
  uint32_t                sender_id_{0};
  std::unique_ptr<EepBase> eep_;
};

}  // namespace enocean_hub
}  // namespace esphome
