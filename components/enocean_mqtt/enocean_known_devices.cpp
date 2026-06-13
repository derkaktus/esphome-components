#include "esphome/components/enocean_mqtt/enocean_known_devices.h"
#include "esphome/core/log.h"

namespace esphome {
namespace enocean_mqtt {

static const char* TAG = "enocean_devices";

void KnownDeviceStore::add(
    const std::string& id,
    const std::string& name,
    const std::string& eep
) {
    // Bereits vorhanden? → updaten
    for (auto& d : devices_) {
        if (d.id == id) {
            d.name = name;
            d.eep  = eep;
            ESP_LOGI(TAG, "Updated device: %s", id.c_str());
            return;
        }
    }
    // Neu anlegen
    devices_.push_back({id, name, eep});
    ESP_LOGI(TAG, "Added device: %s / %s / %s",
        id.c_str(), name.c_str(), eep.c_str());
}

bool KnownDeviceStore::remove(const std::string& id) {
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
        if (it->id == id) {
            ESP_LOGI(TAG, "Removed device: %s", id.c_str());
            devices_.erase(it);
            return true;
        }
    }
    return false;
}

const KnownDevice* KnownDeviceStore::find(const std::string& id) const {
    for (const auto& d : devices_) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

} // namespace enocean_mqtt
} // namespace esphome
