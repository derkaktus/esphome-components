#pragma once
#include "../eep_base.h"

namespace esphome {
namespace enocean_mqtt {

// D5-00-01 - Single Input Contact
// 1BS Telegramm
// Typische Geräte: Türkontakte, Fensterkontakte
class EepD5_00_01 : public EepBase {
public:
    std::string eep_id()      const override { return "D5-00-01"; }
    std::string description() const override { return "Single Input Contact"; }

    bool is_teachin(const uint8_t* data, uint8_t data_len) override {
        // 1BS Teach-In: DB0.Bit3 = 0
        if (data_len >= 1) return !(data[0] & 0x08);
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
