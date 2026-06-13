#include "eep_A5_07_01.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepA5_07_01::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 4) return msgs;

    // DB0.Bit1: PIRS - PIR Status
    // 1 = Bewegung erkannt, 0 = keine Bewegung
    bool motion = (data[3] & 0x02) != 0;

    // DB3: Supply voltage (optional)
    uint8_t raw_voltage = data[0];
    float voltage = (raw_voltage / 255.0f) * 5.0f;

    std::string state = motion ? "motion" : "clear";

    auto json = build_json({
        {"state",   json_str(state)},
        {"motion",  bool_to_str(motion)},
        {"voltage", float_to_str(voltage)}
    });

    add_msg(msgs, base_topic, "state",   state);
    add_msg(msgs, base_topic, "motion",  bool_to_str(motion));
    add_msg(msgs, base_topic, "voltage", float_to_str(voltage));
    add_msg(msgs, base_topic, "json",    json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
