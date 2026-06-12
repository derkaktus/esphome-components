#include "eep_F6_02_01.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepF6_02_01::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 1) return msgs;

    uint8_t db0 = data[0];

    // Bit 4: Energy Bow (1=gedrückt, 0=losgelassen)
    bool pressed = (db0 & 0x10) != 0;

    // Bits 7..5: Button Identifier
    uint8_t button_id = (db0 >> 5) & 0x07;

    std::string button;
    std::string action;

    // Button Mapping laut EnOcean Alliance F6-02-01
    switch (button_id) {
        case 0: button = "AI"; break;  // Rocker 1, oben
        case 1: button = "A0"; break;  // Rocker 1, unten
        case 2: button = "BI"; break;  // Rocker 2, oben
        case 3: button = "B0"; break;  // Rocker 2, unten
        default: button = "unknown"; break;
    }

    action = pressed ? "pressed" : "released";

    // State JSON
    auto json = build_json({
        {"button",  json_str(button)},
        {"action",  json_str(action)},
        {"pressed", bool_to_str(pressed)},
        {"raw",     int_to_str(db0)}
    });

    add_msg(msgs, base_topic, "button", button);
    add_msg(msgs, base_topic, "action", action);
    add_msg(msgs, base_topic, "state",  json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
