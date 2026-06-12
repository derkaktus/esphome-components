#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// A5-06-02 - Light Sensor
// 4BS Telegramm
// Bereich: 0 lx bis 1020 lx
class EepA5_06_02 : public EepBase {
public:
    std::string eep_id()      const override { return "A5-06-02"; }
    std::string description() const override { return "Light Sensor 0-1020 lx"; }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
