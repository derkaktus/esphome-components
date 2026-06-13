#pragma once
#include "eep_parser.h"

namespace esphome {
namespace enocean_mqtt_bridge {

class EepA50205 : public EepParser {
public:
    std::vector<MqttData> parse(const std::vector<uint8_t>& data) override {
        std::vector<MqttData> result;
        // EnOcean A5-02-05 Logik (Beispiel: 3. Byte enthält Temperatur)
        // Data = [Data3, Data2, Data1, Data0]
        if (data.size() >= 4) {
            float temp = 40.0f - ((data[2] * 40.0f) / 255.0f); // Nur als Dummy-Formel
            
            MqttData msg;
            msg.topic_suffix = "temperature";
            msg.payload = std::to_string(temp);
            result.push_back(msg);
        }
        return result;
    }
};

}
}
