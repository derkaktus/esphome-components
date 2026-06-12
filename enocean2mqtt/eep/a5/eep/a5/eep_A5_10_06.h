#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// A5-10-06 - Room Panel
// 4BS Telegramm
// Enthält: Setpoint, Temperature, Fan Speed, Occupancy
class EepA5_10_06 : public EepBase {
public:
    std::string eep_id()      const override { return "A5-10-06"; }
    std::string description() const override { return "Room Panel - Temp/Setpoint/Fan/Occupancy"; }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
