#pragma once

// TODO: copy content here, includes already updated to flat structure

#include <string>
#include <vector>

namespace esphome {
namespace enocean_mqtt {

struct KnownDevice {
    std::string id;
    std::string name;
    std::string eep;
};

class KnownDeviceStore {
public:
    KnownDeviceStore() = default;

    void add(const std::string& id, const std::string& name, const std::string& eep);

    const KnownDevice* find(const std::string& id) const;

    bool remove(const std::string& id);

    std::vector<KnownDevice> all() const;

    size_t count() const { return devices_.size(); }

private:
    std::vector<KnownDevice> devices_;
};

} // namespace enocean_mqtt
} // namespace esphome