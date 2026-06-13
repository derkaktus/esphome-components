#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// A5-07-01 - Occupancy Sensor
// 4BS Telegramm
// Meldet Bewegung / Anwesenheit
class EepA5_07_01 : public EepBase {
public:
    std::string eep_id()      const override { return "A5-07-01"; }
    std::string description() const override { return "Occupancy Sensor"; }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
