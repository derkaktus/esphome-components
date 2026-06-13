#pragma once

// TODO: copy content here, includes already updated to flat structure

#include "eep_base.h"

namespace esphome {
namespace enocean_mqtt {

class EepF6_02_01 : public EepBase {
public:
    bool is_teachin(const uint8_t* data, uint8_t data_len) const override;
    std::vector<EepMessage> parse(const uint8_t* data, uint8_t data_len, const std::string& base_topic) const override;
    std::string eep_id() const override { return "F6:02:01"; }
};

} // namespace enocean_mqtt
} // namespace esphome