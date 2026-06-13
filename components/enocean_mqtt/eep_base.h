#pragma once

// TODO: copy content here, includes already updated to flat structure

#include <string>
#include <vector>

namespace esphome {
namespace enocean_mqtt {

// Forward declaration for message struct
struct EepMessage {
    std::string subtopic;
    std::string payload;
};

// Base class for all EEP parsers
class EepBase {
public:
    virtual ~EepBase() = default;

    // Check if this is a teach-in telegram
    virtual bool is_teachin(const uint8_t* data, uint8_t data_len) const = 0;

    // Parse telegram data and return MQTT messages
    virtual std::vector<EepMessage> parse(
        const uint8_t* data,
        uint8_t data_len,
        const std::string& base_topic
    ) const = 0;

    // Get EEP ID (e.g., "A5:02:05")
    virtual std::string eep_id() const = 0;
};

} // namespace enocean_mqtt
} // namespace esphome