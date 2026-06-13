#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// F6-02-01 - Rocker Switch, 2 Rocker
// RPS Telegramm
// Typische Geräte: Wandschalter, Fernbedienungen
class EepF6_02_01 : public EepBase {
public:
    std::string eep_id()      const override { return "F6-02-01"; }
    std::string description() const override { return "Rocker Switch 2-channel"; }

    bool is_teachin(const uint8_t* data, uint8_t data_len) override {
        // RPS hat kein Teach-In
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
