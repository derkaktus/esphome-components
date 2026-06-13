#pragma once

#include <vector>
#include <string>

namespace esphome {
namespace enocean_mqtt_bridge {

// Hilfsstruktur für die geparsten Daten
struct ParsedMessage {
    std::string topic_suffix; // z.B. "temperature" oder "window_handle"
    std::string payload;      // z.B. "22.5" oder "\"closed\""
};

class EepParser {
public:
    virtual ~EepParser() = default;
    
    // Nimmt die reinen Daten-Bytes (Payload) des EnOcean-Datagramms entgegen
    // und gibt eine Liste von MQTT-Nachrichten zurück
    virtual std::vector<ParsedMessage> parse(const std::vector<uint8_t>& data) = 0;
};

} // namespace enocean_mqtt_bridge
} // namespace esphome
