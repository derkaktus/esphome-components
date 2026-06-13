#include "esphome/components/enocean_mqtt/enocean_mqtt.h"
#include "esphome/components/enocean_mqtt/eep/eep_registry.h"
#include "esphome/core/log.h"

namespace esphome {
namespace enocean_mqtt {

static const char* TAG = "enocean_mqtt";

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::setup() {

    ESP_LOGI(TAG, "EnOcean MQTT Component starting...");
    ESP_LOGI(TAG, "  Main Topic : %s", main_topic_.c_str());
    ESP_LOGI(TAG, "  Debug Raw  : %s", debug_raw_ ? "yes" : "no");
    ESP_LOGI(TAG, "  Known Devs : %d", known_devices_.count());

    // Alle EEP Parser registrieren
    register_all_eeps();

    ESP_LOGI(TAG, "  EEPs loaded: %d",
        (int)EepRegistry::instance().known_eeps().size());

    // MQTT Command Subscription
    // Geräte via MQTT hinzufügen / entfernen
    // Topic: <main_topic>/cmd/#
    if (this->mqtt_ != nullptr) {

        std::string cmd_topic = main_topic_ + "/cmd/#";

        this->mqtt_->subscribe(
            cmd_topic,
            [this](const std::string& topic, const std::string& payload) {
                this->handle_mqtt_command(topic, payload);
            },
            0 // QoS
        );

        ESP_LOGI(TAG, "  Subscribed: %s", cmd_topic.c_str());

        // Online Status publizieren
        mqtt_publish(main_topic_ + "/status", "online", true);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Loop
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::loop() {

    // UART Bytes einlesen
    while (available()) {
        uint8_t byte;
        read_byte(&byte);
        uart_parser_.feed(byte);
    }

    // Vollständige Telegramme verarbeiten
    while (uart_parser_.has_telegram()) {
        auto telegram = uart_parser_.pop_telegram();
        handle_telegram(telegram);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Known Device hinzufügen
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::add_known_device(
    const std::string& id,
    const std::string& name,
    const std::string& eep
) {
    known_devices_.add(id, name, eep);
    ESP_LOGD(TAG, "Known device: %s / %s / %s",
        id.c_str(), name.c_str(), eep.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Telegramm verarbeiten
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::handle_telegram(const EnoceanTelegram& telegram) {

    const std::string& device_id = telegram.sender_id;

    ESP_LOGD(TAG, "Telegram from: %s  RORG: 0x%02X  len: %d",
        device_id.c_str(),
        telegram.rorg,
        telegram.data_len);

    // Debug Raw publizieren
    if (debug_raw_) {
        publish_debug_raw(telegram);
    }

    // Known Device suchen
    const KnownDevice* device = known_devices_.find(device_id);

    // Gerätemetadaten publizieren (Name, EEP)
    publish_device_meta(device_id, device);

    // EEP bestimmen
    std::string eep_str;
    if (device != nullptr) {
        eep_str = device->eep;
    } else {
        // Unbekanntes Gerät: nur Raw Debug, kein EEP Parsing
        ESP_LOGW(TAG, "Unknown device: %s - no EEP parsing",
            device_id.c_str());
        return;
    }

    // EEP dispatchen
    dispatch_eep(eep_str, device_id, telegram.data, telegram.data_len);
}

// ─────────────────────────────────────────────────────────────────────────────
// EEP dispatchen
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::dispatch_eep(
    const std::string& eep,
    const std::string& device_id,
    const uint8_t*     data,
    uint8_t            data_len
) {
    std::string base = device_topic(device_id);

    // Parser aus Registry holen
    EepBase* parser = EepRegistry::instance().get(eep);

    if (parser == nullptr) {
        ESP_LOGW(TAG, "No parser for EEP: %s", eep.c_str());
        mqtt_publish(base + "/eep_error",
            "No parser registered for EEP: " + eep);
        return;
    }

    // Teach-In prüfen
    if (parser->is_teachin(data, data_len)) {
        ESP_LOGI(TAG, "Teach-In from: %s  EEP: %s",
            device_id.c_str(), eep.c_str());
        mqtt_publish(base + "/teach_in", "true");
        return;
    }

    // Parse ausführen
    auto messages = parser->parse(data, data_len, base);

    // Alle MQTT Messages publizieren
    for (const auto& msg : messages) {
        mqtt_publish(msg.subtopic, msg.payload);
    }

    ESP_LOGD(TAG, "Published %d topics for %s [%s]",
        (int)messages.size(),
        device_id.c_str(),
        eep.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Gerätemetadaten publizieren
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::publish_device_meta(
    const std::string&  device_id,
    const KnownDevice*  device
) {
    std::string base = device_topic(device_id);

    if (device != nullptr) {
        // Bekanntes Gerät: Name + EEP publizieren (retained)
        mqtt_publish(base + "/name", device->name, true);
        mqtt_publish(base + "/eep",  device->eep,  true);
    } else {
        // Unbekanntes Gerät: Marker publizieren
        mqtt_publish(base + "/name", "(unknown)", true);
        mqtt_publish(base + "/eep",  "(unknown)", true);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Debug Raw publizieren
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::publish_debug_raw(const EnoceanTelegram& telegram) {

    // Raw Hex String aufbauen
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

// ─────────────────────────────────────────────────────────────────────────────
// MQTT Command Handler
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanMqttComponent::handle_mqtt_command(
    const std::string& topic,
    const std::string& payload
) {
    // Erwartete Topics:
    //   <main_topic>/cmd/add    → payload: "AABBCCDD,Mein Gerät,A5-02-05"
    //   <main_topic>/cmd/remove → payload: "AABBCCDD"
    //   <main_topic>/cmd/list   → payload: (beliebig) → antwortet auf /devices

    std::string prefix = main_topic_ + "/cmd/";

    if (topic.find(prefix) != 0) return;

    std::string cmd = topic.substr(prefix.size());

    // ── ADD ──────────────────────────────────────────────────
    if (cmd == "add") {
        // Format: "DEVICE_ID,Name,EEP"
        std::vector<std::string> parts;
        std::string tmp = payload;
        size_t pos = 0;

        while ((pos = tmp.find(',')) != std::string::npos) {
            parts.push_back(tmp.substr(0, pos));
            tmp.erase(0, pos + 1);
        }
        parts.push_back(tmp); // letztes Element

        if (parts.size() >= 3) {
            std::string id   = parts[0];
            std::string name = parts[1];
            std::string eep  = parts[2];

            // Whitespace trimmen
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

    // ── REMOVE ───────────────────────────────────────────────
    else if (cmd == "remove") {
        std::string id = payload;
        // Whitespace trimmen
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

    // ── LIST ─────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// MQTT Publish Helper
// ─────────────────────────────────────────────────────────────────────────────

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
