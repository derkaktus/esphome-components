#pragma once

#include "eep_parser.h"
#include "esphome/core/helpers.h" // Für to_string

namespace esphome {
namespace enocean_mqtt_bridge {

class EepA50205 : public EepParser {
public:
    std::vector<ParsedMessage> parse(const std::vector<uint8_t>& data) override {
        // Nutzen Sie hier ParsedMessage anstelle von MqttData
        std::vector<ParsedMessage> messages;

        // A5 = 4BS (4 Byte Sensor) Telegramm. 
        // Array muss mind. 5 Bytes haben: R-ORG (0xA5) + 4 Datenbytes (DB3, DB2, DB1, DB0)
        if (data.empty() || data[0] != 0xA5 || data.size() < 5) {
            return messages; 
        }

        // Bei EEP A5-02-05 steckt die Temperatur in Data Byte 1 (DB1).
        // Belegung im Array: data[0]=R-ORG, data[1]=DB3, data[2]=DB2, data[3]=DB1, data[4]=DB0
        uint8_t db1 = data[3];
        
        // Umrechnung laut EnOcean Spezifikation für A5-02-05:
        // Range 0 bis 40 °C. Meist invertiert linear: 0x00 = 40°C, 0xFF = 0°C.
        float temperature = (255 - db1) * (40.0f / 255.0f);

        // Nachricht für MQTT vorbereiten
        ParsedMessage msg;
        msg.topic_suffix = "temperature";
        msg.payload = esphome::to_string(temperature);
        
        messages.push_back(msg);

        return messages;
    }
};

} // namespace enocean_mqtt_bridge
} // namespace esphome
