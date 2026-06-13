#pragma once

// TODO: copy content here, includes already updated to flat structure

#include <string>
#include <vector>
#include <cstdint>

namespace esphome {
namespace enocean_mqtt {

struct EnoceanTelegram {
    std::string sender_id;
    uint8_t     rorg;
    uint8_t     data[64];
    uint8_t     data_len;
    int8_t      rssi;
};

class EnoceanUartParser {
public:
    EnoceanUartParser();

    void feed(uint8_t byte);

    bool has_telegram() const;

    EnoceanTelegram pop_telegram();

private:
    // TODO: implement UART parsing logic
    std::vector<EnoceanTelegram> telegrams_;
};

} // namespace enocean_mqtt
} // namespace esphome