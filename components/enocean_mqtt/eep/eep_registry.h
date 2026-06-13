#pragma once
#include "esphome/components/enocean_mqtt/eep/eep_base.h"
#include <map>
#include <memory>
#include <vector>

namespace esphome {
namespace enocean_mqtt {

class EepRegistry {
public:
    static EepRegistry& instance() {
        static EepRegistry inst;
        return inst;
    }

    void register_eep(std::shared_ptr<EepBase> parser) {
        parsers_[parser->eep_id()] = parser;
    }

    EepBase* get(const std::string& eep_id) {
        auto it = parsers_.find(eep_id);
        if (it != parsers_.end()) return it->second.get();
        return nullptr;
    }

    std::vector<std::string> known_eeps() {
        std::vector<std::string> ids;
        for (auto& p : parsers_) ids.push_back(p.first);
        return ids;
    }

private:
    EepRegistry() = default;
    std::map<std::string, std::shared_ptr<EepBase>> parsers_;
};

// Registriert alle EEPs - Aufruf in setup()
void register_all_eeps();

} // namespace enocean_mqtt
} // namespace esphome
