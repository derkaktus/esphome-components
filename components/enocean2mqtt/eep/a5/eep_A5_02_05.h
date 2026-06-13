#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// A5-02-05 - Temperature Sensor
// 4BS Telegramm
// Bereich: 0°C bis +40°C
class EepA5_02_05 : public EepBase {
public:
    std::string eep_id()      const override { return "A5-02-05"; }
    std::string description() const override { return "Temperature Sensor 0-40°C"; }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
