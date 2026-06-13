#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "eep_parser.h"
#include "eep_A5_02_05.h" // Fügen Sie hier weitere EEPs hinzu

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

// Zustände für unseren ESP3 UART-Parser
enum Esp3State {
    WAIT_SYNC,
    READ_HEADER,
    CHECK_CRC_HEADER,
    READ_DATA,
    CHECK_CRC_DATA
};

class EnOceanBridge : public Component, public uart::UARTDevice {
protected:
    std::map<uint32_t, KnownDevice> known_devices_;

    // --- Variablen für den Zustandsautomaten ---
    Esp3State state_ = WAIT_SYNC;
    std::vector<uint8_t> header_buf_;
    std::vector<uint8_t> data_buf_;
    
    uint16_t data_length_ = 0;
    uint8_t optional_length_ = 0;
    uint8_t packet_type_ = 0;

    // EnOcean nutzt ein spezielles CRC8 (Polynom 0x07)
    uint8_t calc_crc8(uint8_t data, uint8_t crc = 0) {
        crc ^= data;
        for (int i = 0; i < 8; i++) {
            if ((crc & 0x80) != 0)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
        return crc;
    }

    uint8_t calc_crc8_buffer(const std::vector<uint8_t>& buffer) {
        uint8_t crc = 0;
        for (uint8_t b : buffer) {
            crc = calc_crc8(b, crc);
        }
        return crc;
    }

public:
    void add_known_device(uint32_t device_id, const std::string& name, const std::string& eep) {
        KnownDevice dev;
        dev.name = name;
        dev.eep = eep;
        
        // EEP Parser Factory (Hier alle unterstützten EEPs eintragen)
        if (eep == "A5-02-05") {
            dev.parser = std::make_shared<EepA50205>();
        } 
        
        known_devices_[device_id] = dev;
    }

    void loop() override {
        while (this->available()) {
            uint8_t byte;
            this->read_byte(&byte);
            process_serial_byte(byte);
        }
    }

    void process_serial_byte(uint8_t byte) {
        switch (state_) {
            case WAIT_SYNC:
                if (byte == 0x55) { // Sync-Byte erkannt
                    header_buf_.clear();
                    data_buf_.clear();
                    state_ = READ_HEADER;
                }
                break;

            case READ_HEADER:
                header_buf_.push_back(byte);
                if (header_buf_.size() == 4) {
                    state_ = CHECK_CRC_HEADER;
                }
                break;

            case CHECK_CRC_HEADER:
                if (byte == calc_crc8_buffer(header_buf_)) {
                    // Header ist valide! Längen auslesen (Big Endian)
                    data_length_ = (header_buf_[0] << 8) | header_buf_[1];
                    optional_length_ = header_buf_[2];
                    packet_type_ = header_buf_[3];
                    state_ = READ_DATA;
                } else {
                    ESP_LOGW("enocean", "Header CRC Fehler!");
                    state_ = WAIT_SYNC; // Fehler, zurücksetzen
                }
                break;

            case READ_DATA:
                data_buf_.push_back(byte);
                if (data_buf_.size() == (data_length_ + optional_length_)) {
                    state_ = CHECK_CRC_DATA;
                }
                break;

            case CHECK_CRC_DATA:
                if (byte == calc_crc8_buffer(data_buf_)) {
                    // Komplettes Telegramm empfangen!
                    process_telegram(packet_type_, data_buf_);
                } else {
                    ESP_LOGW("enocean", "Data CRC Fehler!");
                }
                state_ = WAIT_SYNC; // Bereit für das nächste Telegramm
                break;
        }
    }

    void process_telegram(uint8_t packet_type, const std::vector<uint8_t>& data) {
        // Wir interessieren uns primär für "Radio ERP1" (Standard Sensoren/Schalter)
        // Packet Type 1 = RADIO_ERP1
        if (packet_type != 0x01 || data_length_ < 6) {
            return; 
        }

        // Bei ERP1 befindet sich die 4-Byte Device-ID (Sender-ID) immer vor dem letzten Byte (Status).
        // Aufbau ERP1 Data: [R-ORG] [Data...] [ID0] [ID1] [ID2] [ID3] [Status]
        int id_offset = data_length_ - 5;
        uint32_t device_id = (data[id_offset] << 24) | 
                             (data[id_offset + 1] << 16) | 
                             (data[id_offset + 2] << 8) | 
                             (data[id_offset + 3]);

        // Nur die reinen ERP1 Daten extrahieren (ohne Optional Data am Ende)
        std::vector<uint8_t> erp1_data(data.begin(), data.begin() + data_length_);

        handle_telegram(device_id, erp1_data);
    }

    void handle_telegram(uint32_t device_id, const std::vector<uint8_t>& data) {
        char id_str[20];
        sprintf(id_str, "%08X", device_id);

        std::string base_topic = "enocean/" + std::string(id_str);
        
        auto it = known_devices_.find(device_id);
        if (it != known_devices_.end()) {
            KnownDevice& dev = it->second;
            
            if (dev.parser) {
                auto messages = dev.parser->parse(data);
                for (auto& msg : messages) {
                    std::string topic = base_topic + "/" + msg.topic_suffix;
                    std::string payload = "{\"name\":\"" + dev.name + "\",\"eep\":\"" + dev.eep + "\",\"value\":" + msg.payload + "}";
                    
                    if (mqtt::global_mqtt_client != nullptr) {
                        mqtt::global_mqtt_client->publish(topic, payload);
                    }
                }
            } else {
                ESP_LOGW("enocean", "Parser für EEP %s nicht implementiert (Gerät %s)", dev.eep.c_str(), id_str);
            }
        } else {
            // Unbekanntes Gerät
            std::string payload = "{\"name\":\"\",\"eep\":\"\",\"value\":\"\"}";
            if (mqtt::global_mqtt_client != nullptr) {
                mqtt::global_mqtt_client->publish(base_topic + "/unknown", payload);
            }
            ESP_LOGD("enocean", "Unbekanntes Gerät empfangen: %s", id_str);
        }
    }
};

}
}
