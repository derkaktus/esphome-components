#include "enocean_mqtt.h"
#include "eep_registry.h"
#include "esphome/core/log.h"

namespace esphome {
namespace enocean_mqtt {

static const char* TAG = "enocean_mqtt";

void EnoceanMqttComponent::setup() {

    ESP_LOGI(TAG, "EnOcean MQTT Component starting...");
    ESP_LOGI(TAG, "  Main Topic : %s", main_topic_.c_str());
    ESP_LOGI(TAG, "  Debug Raw  : %s", debug_raw_ ? "yes" : "no");
    ESP_LOGI(TAG, "  Known Devs : %d", known_devices_.count());

    register_all_eeps();

    ESP_LOGI(TAG, "  EEPs loaded: %d",
        (int)EepRegistry::instance().known_eeps().size());

    if (this->mqtt_ != nullptr) {

        std::string cmd_topic = main_topic_ + "/cmd/#";

        this->mqtt_->subscribe(
            cmd_topic,
            [this](const std::string& topic, const std::string& payload) {
                this->handle_mqtt_command(topic, payload);
            },
            0
        );

        ESP_LOGI(TAG, "  Subscribed: %s", cmd_topic.c_str());

        mqtt_publish(main_topic_ + "/status", "online", true);
    }
}

void EnoceanMqttComponent::loop() {

    while (available()) {
        uint8_t byte;
        read_byte(&byte);
        uart_parser_.feed(byte);
    }

    while (uart_parser_.has_telegram()) {
        auto telegram = uart_parser_.pop_telegram();
        handle_telegram(telegram);
    }
}

void EnoceanMqttComponent::add_known_device(
    const std::string& id,
    const std::string& name,
    const std::string& eep
) {
    known_devices_.add(id, name, eep);
    ESP_LOGD(TAG, "Known device: %s / %s / %s",
        id.c_str(), name.c_str(), eep.c_str());
}

void EnoceanMqttComponent::handle_telegram(const EnoceanTelegram& telegram) {

    const std::string& device_id = telegram.sender_id;

    ESP_LOGD(TAG, "Telegram from: %s  RORG: 0x%02X  len: %d",
        device_id.c_str(),
        telegram.rorg,
        telegram.data_len);

    if (debug_raw_) {
        publish_debug_raw(telegram);
    }

    const KnownDevice* device = known_devices_.find(device_id);

    publish_device_meta(device_id, device);

    std::string eep_str;
    if (device != nullptr) {
        eep_str = device->eep;
    } else {
        ESP_LOGW(TAG, "Unknown device: %s - no EEP parsing",
            device_id.c_str());
        return;
    }

    dispatch_eep(eep_str, device_id, telegram.data, telegram.data_len);
}

void EnoceanMqttComponent::dispatch_eep(
    const std::string& eep,
    const std::string& device_id,
    const uint8_t*     data,
    uint8_t            data_len
) {
    std::string base = device_topic(device_id);

    EepBase* parser = EepRegistry::instance().get(eep);

    if (parser == nullptr) {
        ESP_LOGW(TAG, "No parser for EEP: %s", eep.c_str());
        mqtt_publish(base + "/eep_error",
            "No parser registered for EEP: " + eep);
        return;
    }

    if (parser->is_teachin(data, data_len)) {
        ESP_LOGI(TAG, "Teach-In from: %s  EEP: %s",
            device_id.c_str(), eep.c_str());
        mqtt_publish(base + "/teach_in", "true");
        return;
    }

    auto messages = parser->parse(data, data_len, base);

    for (const auto& msg : messages) {
        mqtt_publish(msg.subtopic, msg.payload);
    }

    ESP_LOGD(TAG, "Published %d topics for %s [%s]",
        (int)messages.size(),
        device_id.c_str(),
        eep.c_str());
}

void EnoceanMqttComponent::publish_device_meta(
    const std::string&  device_id,
    const KnownDevice*  device
) {
    std::string base = device_topic(device_id);

    if (device != nullptr) {
        mqtt_publish(base + "/name", device->name, true);
        mqtt_publish(base + "/eep",  device->eep,  true);
    } else {
        mqtt_publish(base + "/name", "(unknown)", true);
        mqtt_publish(base + "/eep",  "(unknown)", true);
    }
}

void EnoceanMqttComponent::publish_debug_raw(const EnoceanTelegram& telegram) {

    char buf[256] = {0};
    int  pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "{\"sender\":\"%s\",\"rorg\":\"0x%02X\",\"data\":\"",
        telegram.sender_id.c_str(),
        telegram.rorg);

    for (int i = 0; i < telegram.data_len && pos < (int)sizeof(buf) - 8; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%02X", telegram.data[i]);
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos,
        "\",\"rssi\":%d}",
        telegram.rssi);

    mqtt_publish(main_topic_ + "/debug/raw", std::string(buf));
}

void EnoceanMqttComponent::handle_mqtt_command(
    const std::string& topic,
    const std::string& payload
) {
    std::string prefix = main_topic_ + "/cmd/";

    if (topic.find(prefix) != 0) return;

    std::string cmd = topic.substr(prefix.size());

    if (cmd == "add") {
        std::vector<std::string> parts;
        std::string tmp = payload;
        size_t pos = 0;

        while ((pos = tmp.find(',')) != std::string::npos) {
            parts.push_back(tmp.substr(0, pos));
            tmp.erase(0, pos + 1);
        }
        parts.push_back(tmp);

        if (parts.size() >= 3) {
            std::string id   = parts[0];
            std::string name = parts[1];
            std::string eep  = parts[2];

            auto trim = [](std::string& s) {
                while (!s.empty() && (s.front() == ' ' || s.front() == '\r'))
                    s.erase(s.begin());
                while (!s.empty() && (s.back() == ' ' || s.back() == '\r'))
                    s.pop_back();
            };
            trim(id); trim(name); trim(eep);

            known_devices_.add(id, name, eep);

            ESP_LOGI(TAG, "CMD add: %s / %s / %s",
                id.c_str(), name.c_str(), eep.c_str());

            mqtt_publish(main_topic_ + "/cmd/result",
                "added: " + id + " / " + name + " / " + eep);
        } else {
            ESP_LOGW(TAG, "CMD add: invalid payload: %s", payload.c_str());
            mqtt_publish(main_topic_ + "/cmd/result",
                "error: format must be ID,Name,EEP");
        }
    }

    else if (cmd == "remove") {
        std::string id = payload;
        while (!id.empty() && (id.back() == ' ' || id.back() == '\r'))
            id.pop_back();

        bool removed = known_devices_.remove(id);

        if (removed) {
            ESP_LOGI(TAG, "CMD remove: %s", id.c_str());
            mqtt_publish(main_topic_ + "/cmd/result", "removed: " + id);
        } else {
            ESP_LOGW(TAG, "CMD remove: not found: %s", id.c_str());
            mqtt_publish(main_topic_ + "/cmd/result",
                "error: device not found: " + id);
        }
    }

    else if (cmd == "list") {
        auto all = known_devices_.all();
        std::string json = "[";

        for (size_t i = 0; i < all.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"id\":\"" + all[i].id +
                    "\",\"name\":\"" + all[i].name +
                    "\",\"eep\":\"" + all[i].eep + "\"}";
        }
        json += "]";

        mqtt_publish(main_topic_ + "/devices", json, true);
        ESP_LOGI(TAG, "CMD list: %d devices", (int)all.size());
    }

    else {
        ESP_LOGW(TAG, "CMD unknown: %s", cmd.c_str());
    }
}

void EnoceanMqttComponent::mqtt_publish(
    const std::string& topic,
    const std::string& payload,
    bool retain
) {
    if (this->mqtt_ == nullptr) return;

    this->mqtt_->publish(topic, payload, 0, retain);

    ESP_LOGV(TAG, "MQTT → %s : %s", topic.c_str(), payload.c_str());
}

} // namespace enocean_mqtt
} // namespace esphome