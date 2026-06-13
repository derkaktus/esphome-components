#pragma once
#include <string>
#include <vector>
#include <map>

namespace esphome {
namespace enocean_mqtt_bridge {

// Struktur für die extrahierten MQTT-Daten
struct MqttData {
    std::string topic_suffix;
    std::string payload; // oder JSON format
};

class EepParser {
public:
    virtual ~EepParser() = default;
    // Nimmt die reinen EnOcean-Datenbytes und gibt eine Liste von MQTT-Nachrichten zurück
    virtual std::vector<MqttData> parse(const std::vector<uint8_t>& data) = 0;
};

} // namespace
} // namespace
