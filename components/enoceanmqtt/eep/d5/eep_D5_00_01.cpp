#include "eep_D5_00_01.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepD5_00_01::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 1) return msgs;

    uint8_t db0 = data[0];

    // DB0.Bit0: CO (Contact)
    // 1 = offen, 0 = geschlossen
    bool contact_open = (db0 & 0x01) != 0;
    std::string state = contact_open ? "open" : "closed";

    auto json = build_json({
        {"state",        json_str(state)},
        {"contact_open", bool_to_str(contact_open)}
    });

    add_msg(msgs, base_topic, "state", state);
    add_msg(msgs, base_topic, "json",  json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
