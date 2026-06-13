#include "eep_A5_10_06.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepA5_10_06::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 4) return msgs;

    // DB3: Setpoint
    // Skalierung: 0x00 = min, 0xFF = max (gerätespezifisch, typisch 0-40°C)
    uint8_t raw_setpoint = data[0];
    float setpoint = (raw_setpoint / 255.0f) * 40.0f;

    // DB2: Temperature
    // Skalierung: 0x00 = 0°C, 0xFF = 40°C
    uint8_t raw_temp = data[1];
    float temperature = (raw_temp / 255.0f) * 40.0f;

    // DB1: Fan Speed
    // 0x00 = Auto, 0x01 = Speed 1, 0x02 = Speed 2, 0x03 = Speed 3
    uint8_t fan_raw = data[2];
    std::string fan_speed;
    switch (fan_raw) {
        case 0x00: fan_speed = "auto";    break;
        case 0x01: fan_speed = "speed_1"; break;
        case 0x02: fan_speed = "speed_2"; break;
        case 0x03: fan_speed = "speed_3"; break;
        default:   fan_speed = "unknown"; break;
    }

    // DB0.Bit0: PIRS - Occupancy
    bool occupied = (data[3] & 0x01) != 0;

    auto json = build_json({
        {"setpoint",    float_to_str(setpoint)},
        {"temperature", float_to_str(temperature)},
        {"fan_speed",   json_str(fan_speed)},
        {"occupied",    bool_to_str(occupied)}
    });

    add_msg(msgs, base_topic, "setpoint",    float_to_str(setpoint));
    add_msg(msgs, base_topic, "temperature", float_to_str(temperature));
    add_msg(msgs, base_topic, "fan_speed",   fan_speed);
    add_msg(msgs, base_topic, "occupied",    bool_to_str(occupied));
    add_msg(msgs, base_topic, "state",       json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
