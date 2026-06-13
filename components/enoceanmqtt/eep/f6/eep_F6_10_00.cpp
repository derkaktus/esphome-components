#include "eep_F6_10_00.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepF6_10_00::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 1) return msgs;

    uint8_t status       = data[0];
    uint8_t upper_nibble = status & 0xF0;
    std::string state;

    // EnOcean Alliance Standard
    // DB0 oberes Nibble definiert Fensterstellung
    switch (upper_nibble) {
        case 0xF0: state = "closed"; break;
        case 0xE0: state = "open";   break;
        case 0xD0: state = "tilted"; break;
        default:   state = "unknown"; break;
    }

    auto json = build_json({
        {"state",   json_str(state)},
        {"raw_hex", json_str(byte_to_hex(status))}
    });

    add_msg(msgs, base_topic, "state",   state);
    add_msg(msgs, base_topic, "raw_hex", byte_to_hex(status));
    add_msg(msgs, base_topic, "json",    json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
