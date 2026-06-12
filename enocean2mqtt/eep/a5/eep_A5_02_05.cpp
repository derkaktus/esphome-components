#include "eep_A5_02_05.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepA5_02_05::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 4) return msgs;

    // DB1 = Temperatur Rohdaten
    // Skalierung: 0x00 = 40°C, 0xFF = 0°C (invertiert!)
    uint8_t raw_temp = data[1];
    float temperature = 40.0f - (raw_temp / 255.0f * 40.0f);

    auto json = build_json({
        {"temperature", float_to_str(temperature)},
        {"unit",        json_str("°C")}
    });

    add_msg(msgs, base_topic, "temperature", float_to_str(temperature));
    add_msg(msgs, base_topic, "state",       json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
