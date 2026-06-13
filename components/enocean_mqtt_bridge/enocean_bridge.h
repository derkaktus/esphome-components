#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/mqtt/mqtt_client.h" // Für MQTT
#include "eep_parser.h"
#include "eep_A5_02_05.h"

#include <map>
#include <vector>
#include <memory>

namespace esphome {
namespace enocean_mqtt_bridge {

struct KnownDevice {
    std::string name;
    std::string eep;
    std::shared_ptr<EepParser> parser;
};

class EnOceanBridge : public Component, public uart::UARTDevice {
protected:
    std::map<uint32_t, KnownDevice> known_devices_;

public:
    void add_known_device(uint32_t device_id, const std::string& name, const std::string& eep) {
        KnownDevice dev;
        dev.name = name;
        dev.eep = eep;
        
        // EEP Parser Factory
        if (eep == "A5-02-05") {
            dev.parser = std::make_shared<EepA50205>();
        } 
        // else if (eep == "F6-02-01") { ... }
        
        known_devices_[device_id] = dev;
    }

    void loop() override {
        // 1. Lese UART (TCM310 ESP3 Protokoll)
        while (this->available()) {
            uint8_t byte;
            this->read_byte(&byte);
            
            // HIER: Logik zum Zusammensetzen des ESP3 Frames (Sync 0x55, Header, CRC8, Data) implementieren!
            // Wenn Frame komplett: handle_telegram(device_id, data_bytes);
        }
    }

    void handle_telegram(uint32_t device_id, const std::vector<uint8_t>& data) {
        char id_str[20];
        sprintf(id_str, "%08X", device_id);

        std::string base_topic = "enocean/" + std::string(id_str);
        
        // Prüfen, ob das Gerät bekannt ist
        auto it = known_devices_.find(device_id);
        if (it != known_devices_.end()) {
            // Gerät IST bekannt
            KnownDevice& dev = it->second;
            
            if (dev.parser) {
                auto messages = dev.parser->parse(data);
                for (auto& msg : messages) {
                    // Topic: enocean/<ID>/<Channel>
                    std::string topic = base_topic + "/" + msg.topic_suffix;
                    
                    // Payload z.B. als JSON formatieren
                    std::string payload = "{\"name\":\"" + dev.name + "\",\"eep\":\"" + dev.eep + "\",\"value\":" + msg.payload + "}";
                    
                    if (mqtt::global_mqtt_client != nullptr) {
                        mqtt::global_mqtt_client->publish(topic, payload);
                    }
                }
            }
        } else {
            // Gerät UNBEKANNT
            std::string payload = "{\"name\":\"\",\"eep\":\"\",\"raw_data\":\"...\"}"; // raw_data als Hex
            if (mqtt::global_mqtt_client != nullptr) {
                mqtt::global_mqtt_client->publish(base_topic + "/unknown", payload);
            }
        }
    }
};

}
}
