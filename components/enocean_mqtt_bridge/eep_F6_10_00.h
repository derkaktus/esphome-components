#pragma once

#include "eep_parser.h"
#include <string>

namespace esphome {
namespace enocean_mqtt_bridge {

class EepF61000 : public EepParser {
public:
    std::vector<ParsedMessage> parse(const std::vector<uint8_t>& data) override {
        std::vector<ParsedMessage> messages;

        // F6 = RPS Telegramm. Wir prüfen zur Sicherheit, ob das R-ORG Byte stimmt.
        if (data.empty() || data[0] != 0xF6) {
            return messages; // Falsches Telegramm für diesen Parser
        }

        // Bei RPS-Telegrammen ist data[1] das entscheidende Datenbyte (Data Byte 0)
        uint8_t data_byte = data[1];
        std::string state_str = "unknown";

        // HOPPE Fenstergriff Zustände (SecuSignal)
        if (data_byte == 0xF0) {
            state_str = "\"closed\"";
        } else if (data_byte == 0xE0) {
            state_str = "\"open\"";
        } else if (data_byte == 0xC0) {
            state_str = "\"tilted\"";
        } else {
            // Falls Zwischenwerte auftreten, als reinen Hex-String ausgeben
            char raw_hex[10];
            sprintf(raw_hex, "\"0x%02X\"", data_byte);
            state_str = raw_hex;
        }

        // Nachricht für MQTT vorbereiten
        ParsedMessage msg;
        msg.topic_suffix = "window_handle";
        msg.payload = state_str; // Da es Strings sind, sind die Quotes (\") schon im String!
        
        messages.push_back(msg);

        return messages;
    }
};

} // namespace enocean_mqtt_bridge
} // namespace esphome
