#pragma once
#include <string>
#include <vector>
#include <map>

namespace esphome {
namespace enocean_mqtt {

struct MqttMessage {
    std::string subtopic;
    std::string payload;
};

class EepBase {
public:
    virtual ~EepBase() = default;
    virtual std::string eep_id()      const = 0;
    virtual std::string description() const = 0;

    virtual std::vector<MqttMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) = 0;

    virtual bool is_teachin(const uint8_t* data, uint8_t data_len) {
        if (data_len >= 4) return !(data[3] & 0x08);
        return false;
    }

protected:
    void add_msg(
        std::vector<MqttMessage>& msgs,
        const std::string& base_topic,
        const std::string& subtopic,
        const std::string& payload
    ) {
        msgs.push_back({base_topic + "/" + subtopic, payload});
    }

    std::string float_to_str(float val, int decimals = 1) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        return std::string(buf);
    }

    std::string int_to_str(int val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", val);
        return std::string(buf);
    }

    std::string bool_to_str(bool val) {
        return val ? "1" : "0";
    }

    std::string byte_to_hex(uint8_t val) {
        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02X", val);
        return std::string(buf);
    }

    // JSON Builder - einfache Key/Value Paare
    std::string build_json(const std::vector<std::pair<std::string, std::string>>& fields) {
        std::string json = "{";
        for (size_t i = 0; i < fields.size(); i++) {
            if (i > 0) json += ",";
            json += "\"" + fields[i].first + "\":" + fields[i].second;
        }
        json += "}";
        return json;
    }

    // JSON String Wert (mit Anführungszeichen)
    std::string json_str(const std::string& val) {
        return "\"" + val + "\"";
    }
};

} // namespace enocean_mqtt
} // namespace esphome
