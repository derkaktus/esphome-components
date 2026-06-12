#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// A5-20-06 - HVAC Actuator - Energy Harvesting
// 4BS Telegramm
// Micropelt MVA009 / ähnliche Harvesting Ventilantriebe
class EepA5_20_06 : public EepBase {
public:
    std::string eep_id()      const override { return "A5-20-06"; }
    std::string description() const override { return "HVAC Actuator - Energy Harvesting (Micropelt MVA009)"; }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
