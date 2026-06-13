#include "eep_registry.h"

#include "eep_F6_02_01.h"
#include "eep_F6_10_00.h"
#include "eep_A5_02_05.h"
#include "eep_A5_06_02.h"
#include "eep_A5_07_01.h"
#include "eep_A5_09_04.h"
#include "eep_A5_10_06.h"
#include "eep_A5_20_06.h"
#include "eep_D5_00_01.h"

#include "esphome/core/log.h"

namespace esphome {
namespace enocean_mqtt {

static const char* TAG = "eep_registry";

void EepRegistry::register_eep(const std::string& eep_id, std::unique_ptr<EepBase> parser) {
    if (parsers_.find(eep_id) != parsers_.end()) {
        ESP_LOGW(TAG, "Duplicate EEP registered: %s", eep_id.c_str());
        return;
    }

    parsers_[eep_id] = std::move(parser);
    known_eeps_.push_back(eep_id);

    ESP_LOGD(TAG, "Registered EEP: %s", eep_id.c_str());
}

EepBase* EepRegistry::get(const std::string& eep_id) const {
    auto it = parsers_.find(eep_id);
    if (it == parsers_.end()) {
        return nullptr;
    }
    return it->second.get();
}

void register_all_eeps() {
    EepRegistry::instance().register_eep("F6:02:01", std::make_unique<EepF6_02_01>());
    EepRegistry::instance().register_eep("F6:10:00", std::make_unique<EepF6_10_00>());
    EepRegistry::instance().register_eep("A5:02:05", std::make_unique<EepA5_02_05>());
    EepRegistry::instance().register_eep("A5:06:02", std::make_unique<EepA5_06_02>());
    EepRegistry::instance().register_eep("A5:07:01", std::make_unique<EepA5_07_01>());
    EepRegistry::instance().register_eep("A5:09:04", std::make_unique<EepA5_09_04>());
    EepRegistry::instance().register_eep("A5:10:06", std::make_unique<EepA5_10_06>());
    EepRegistry::instance().register_eep("A5:20:06", std::make_unique<EepA5_20_06>());
    EepRegistry::instance().register_eep("D5:00:01", std::make_unique<EepD5_00_01>());
}

} // namespace enocean_mqtt
} // namespace esphome