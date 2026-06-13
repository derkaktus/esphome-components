#pragma once
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

    void add(
        const std::string& id,
        const std::string& name,
        const std::string& eep
    );

    bool remove(const std::string& id);

    const KnownDevice* find(const std::string& id) const;

    std::vector<KnownDevice> all() const { return devices_; }

    int count() const { return (int)devices_.size(); }

private:
    std::vector<KnownDevice> devices_;
};

} // namespace enocean_mqtt
} // namespace esphome
