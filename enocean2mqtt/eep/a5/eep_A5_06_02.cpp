#include "eep_A5_06_02.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepA5_06_02::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 4) return msgs;

    // DB2 = Licht Rohdaten
    // Skalierung: 0x00 = 0 lx, 0xFF = 1020 lx
    uint8_t raw_light = data[2];
    float illuminance = (raw_light / 255.0f) * 1020.0f;

    // DB0.Bit0: Supply voltage ok
    bool voltage_ok = (data[3] & 0x01) != 0;

    auto json = build_json({
        {"illuminance", float_to_str(illuminance, 0)},
        {"unit",        json_str("lx")},
        {"voltage_ok",  bool_to_str(voltage_ok)}
    });

    add_msg(msgs, base_topic, "illuminance", float_to_str(illuminance, 0));
    add_msg(msgs, base_topic, "voltage_ok",  bool_to_str(voltage_ok));
    add_msg(msgs, base_topic, "state",       json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
