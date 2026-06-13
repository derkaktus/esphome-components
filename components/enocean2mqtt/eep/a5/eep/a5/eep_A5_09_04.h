#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// A5-09-04 - CO2 / Humidity / Temperature Sensor
// 4BS Telegramm
// Kombinierter Raumluftqualitätssensor
class EepA5_09_04 : public EepBase {
public:
    std::string eep_id()      const override { return "A5-09-04"; }
    std::string description() const override { return "CO2 / Humidity / Temperature Sensor"; }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
