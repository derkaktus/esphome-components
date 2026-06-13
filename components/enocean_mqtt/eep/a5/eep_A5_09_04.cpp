#include "eep_A5_09_04.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepA5_09_04::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 4) return msgs;

    // DB3: Humidity
    // Skalierung: 0x00 = 0%, 0xC8 = 100%
    uint8_t raw_hum = data[0];
    float humidity = (raw_hum / 200.0f) * 100.0f;

    // DB2: Temperature
    // Skalierung: 0x00 = 0°C, 0xFF = 51°C
    uint8_t raw_temp = data[1];
    float temperature = (raw_temp / 255.0f) * 51.0f;

    // DB1 + DB0 high bits: CO2
    // Skalierung: 0x000 = 0 ppm, 0x7FF = 2550 ppm
    uint16_t raw_co2 = ((uint16_t)data[2] << 3) | (data[3] >> 5);
    float co2 = (raw_co2 / 2047.0f) * 2550.0f;

    // DB0.Bit2: HT - Humidity/Temperature available
    bool ht_available = (data[3] & 0x04) != 0;

    auto json = build_json({
        {"co2",          float_to_str(co2, 0)},
        {"temperature",  float_to_str(temperature)},
        {"humidity",     float_to_str(humidity)},
        {"ht_available", bool_to_str(ht_available)}
    });

    add_msg(msgs, base_topic, "co2",         float_to_str(co2, 0));
    add_msg(msgs, base_topic, "temperature", float_to_str(temperature));
    add_msg(msgs, base_topic, "humidity",    float_to_str(humidity));
    add_msg(msgs, base_topic, "state",       json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
