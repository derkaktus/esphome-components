#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdio>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

// Inkludieren der EEPs und Parser-Schablone
#include "eep_parser.h"
#include "eep_A5_02_05.h"
#include "eep_F6_10_00.h"

namespace esphome {
namespace enocean_mqtt_bridge {

static const char *const TAG = "enocean_bridge";

struct KnownDevice {
    uint32_t device_id;
    std::string name;
    std::string eep;
    std::shared_ptr<EepParser> parser;
};

class EnOceanBridge : public uart::UARTDevice, public Component {
private:
    std::vector<KnownDevice> known_devices_;
    bool parse_all_dev_{false};
    std::vector<uint8_t> rx_buffer_;

    // ESP3 Protokoll-Konstanten
    static const uint8_t ESP3_SYNC_BYTE = 0x55;
    static const uint8_t PACKET_TYPE_RADIO = 0x01; // ERP1 (Standard EnOcean Funk)

public:
    void set_parse_all_dev(bool parse_all) {
        this->parse_all_dev_ = parse_all;
    }

    void add_known_device(uint32_t device_id, const std::string &name, const std::string &eep) {
        std::shared_ptr<EepParser> parser = nullptr;

        // Parser zuweisen anhand der EEP
        if (eep == "A5-02-05") {
            parser = std::make_shared<EepA50205>();
        } else if (eep == "F6-10-00") {
            parser = std::make_shared<EepF61000>();
        } else {
            ESP_LOGW(TAG, "Unbekanntes EEP %s für Gerät %08X", eep.c_str(), device_id);
        }

        this->known_devices_.push_back({device_id, name, eep, parser});
    }

    void setup() override {
        ESP_LOGI(TAG, "EnOcean Bridge Setup gestartet. Parse All Dev: %s", parse_all_dev_ ? "true" : "false");
    }

    void loop() override {
        // Daten vom UART (TCM310) einlesen
        while (this->available()) {
            uint8_t byte;
            this->read_byte(&byte);
            rx_buffer_.push_back(byte);

            // Sehr simples Buffering: Wir suchen das Sync-Byte 0x55
            if (rx_buffer_[0] != ESP3_SYNC_BYTE) {
                rx_buffer_.erase(rx_buffer_.begin());
                continue;
            }

            // Ein ESP3 Header hat 4 Bytes + 1 Byte CRC8 = 6 Bytes (inkl. Sync)
            if (rx_buffer_.size() >= 6) {
                uint16_t data_length = (rx_buffer_[1] << 8) | rx_buffer_[2];
                uint8_t option_length = rx_buffer_[3];
                uint8_t packet_type = rx_buffer_[4];
                
                // Gesamtlänge = Sync + Header + CRC8H + Data + Option + CRC8D
                size_t total_length = 6 + data_length + option_length + 1;

                if (rx_buffer_.size() >= total_length) {
                    // Komplettes Paket empfangen!
                    // Wenn es ein reguläres Funktelegramm ist (ERP1)
                    if (packet_type == PACKET_TYPE_RADIO) {
                        // Datenbereich extrahieren (beginnt ab Index 6)
                        std::vector<uint8_t> payload_data(rx_buffer_.begin() + 6, rx_buffer_.begin() + 6 + data_length);
                        process_radio_packet(payload_data);
                    }
                    
                    // Puffer aufräumen
                    rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + total_length);
                }
            }
        }
    }

private:
    void process_radio_packet(const std::vector<uint8_t>& data) {
        if (data.size() < 5) return; // Zu kurz für ein gültiges EnOcean ERP1 Telegramm

        // Bei ERP1 steckt die Sender-ID in den letzten 5 Bytes (vor dem Status-Byte)
        // [R-ORG (1)] [Data (1-4)] [SenderID (4)] [Status (1)]
        size_t id_offset = data.size() - 5;
        uint32_t sender_id = (data[id_offset] << 24) | 
                             (data[id_offset + 1] << 16) | 
                             (data[id_offset + 2] << 8) | 
                             data[id_offset + 3];

        char id_str[9];
        sprintf(id_str, "%08X", sender_id);

        bool device_known = false;

        for (auto &dev : known_devices_) {
            if (dev.device_id == sender_id) {
                device_known = true;
                if (dev.parser != nullptr) {
                    auto messages = dev.parser->parse(data);
                    for (const auto &msg : messages) {
                        publish_mqtt(id_str, dev.name, dev.eep, msg.topic_suffix, msg.payload);
                    }
                } else {
                    ESP_LOGE(TAG, "Kein Parser für Gerät %s hinterlegt!", id_str);
                }
                break;
            }
        }

        // Unbekanntes Gerät
        if (!device_known && this->parse_all_dev_) {
            std::string raw_hex = esphome::format_hex(data.data(), data.size());
            // Für unbekannte Geräte: name und eep sind leer, suffix ist "raw_payload"
            publish_mqtt(id_str, "", "", "raw_payload", raw_hex);
            ESP_LOGD(TAG, "Unbekanntes Gerät %s: raw_payload an MQTT gesendet (%s)", id_str, raw_hex.c_str());
        }
    }

    void publish_mqtt(const std::string& device_id, const std::string& name, const std::string& eep, const std::string& topic_suffix, const std::string& payload) {
        if (esphome::mqtt::global_mqtt_client == nullptr || !esphome::mqtt::global_mqtt_client->is_connected()) {
            ESP_LOGW(TAG, "MQTT nicht verbunden, verwerfe Nachricht für %s", device_id.c_str());
            return;
        }

        // Topic-Aufbau wie gefordert: enocean/deviceid/name,eep,suffix
        // Wenn name und eep leer sind (unbekanntes Gerät), sieht es so aus: enocean/deviceid/,,raw_payload
        std::string topic = "enocean/" + device_id + "/" + name + "," + eep + "," + topic_suffix;
        
        esphome::mqtt::global_mqtt_client->publish(topic, payload);
    }
};

} // namespace enocean_mqtt_bridge
} // namespace esphome
