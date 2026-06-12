#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// F6-10-00 - Window Handle
// RPS Telegramm
// Zustände: closed / open / tilted
class EepF6_10_00 : public EepBase {
public:
    std::string eep_id()      const override { return "F6-10-00"; }
    std::string description() const override { return "Window Handle"; }

    bool is_teachin(const uint8_t* data, uint8_t data_len) override {
        return false;
    }

    std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) override;
};

} // namespace enocean_mqtt
} // namespace esphome
