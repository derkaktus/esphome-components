#pragma once

#include "eep_base.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace esphome {
namespace enocean_mqtt {

// Forward declaration
class EepBase;

// EEP Registry: singleton that holds all registered EEP parsers
class EepRegistry {
public:
    static EepRegistry& instance() {
        static EepRegistry instance_;
        return instance_;
    }

    // Register an EEP parser (takes ownership via unique_ptr)
    void register_eep(const std::string& eep_id, std::unique_ptr<EepBase> parser);

    // Get a parser by EEP ID
    EepBase* get(const std::string& eep_id) const;

    // List all registered EEPs
    const std::vector<std::string>& known_eeps() const { return known_eeps_; }

private:
    EepRegistry() = default;
    std::map<std::string, std::unique_ptr<EepBase>> parsers_;
    std::vector<std::string> known_eeps_;
};

// Function to register all known EEPs (called at startup)
void register_all_eeps();

} // namespace enocean_mqtt
} // namespace esphome