#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/preferences.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>
#include <cstring>

namespace esphome {
namespace enocean {

static const char *const TAG = "enocean_bridge";

// Die Struktur für den dauerhaften NVS-Speicher (Exakt 52 Bytes)
struct SavedDevice {
    uint32_t device_id;
    char name[32];
    char eep[16];
};

// Hilfsstruktur für die temporäre Speicherung der YAML-Geräte vor dem Setup
struct YamlDevice {
    uint32_t device_id;
    std::string name;
    std::string eep;
};

// Hauptklasse der EnOcean Bridge
class EnOceanBridge : public uart::UARTDevice, public Component {
public:
    // Konfigurations-Setter (aufgerufen durch die Python __init__.py)
    void set_parse_all_dev(bool parse_all) { this->parse_all_dev_ = parse_all; }
    
    // NEU: Setzt die maximale Anzahl an erlaubten Geräten im Flash
    void set_max_dev(uint32_t max_dev) { this->max_dev_ = max_dev; }

    // NEU: Nimmt Geräte aus der yaml-Konfig entgegen und speichert sie temporär
    void add_yaml_device(uint32_t device_id, const std::string &name, const std::string &eep) {
        this->yaml_devices_.push_back({device_id, name, eep});
    }

    void setup() override {
        ESP_LOGI(TAG, "Starte EnOcean Bridge (Max. Geräte im Flash: %u)...", this->max_dev_);

        // Registrierung des Flash-Speicher-Bereichs mit dynamischer, berechneter Größe
        // Speichergröße = sizeof(size_t) für die Anzahl + (max_dev * 52 Bytes pro Gerät)
        size_t pref_size = sizeof(size_t) + (this->max_dev_ * sizeof(SavedDevice));
        //this->pref_ = global_preferences->make_preference<uint8_t[]>(fnv1_hash("enocean_devices"), pref_size);
        this->pref_ = global_preferences->make_preference(pref_size, fnv1_hash("enocean_devices")); 
        // NVS Speicher laden
        load_from_nvs();

        // NEU: Synchronisation der YAML-Geräte in den NVS Flash
        sync_yaml_to_nvs();
    }

    void loop() override {
        // Hier läuft die serielle UART-Auswertung der TCM310-Telegramme
        // und die Weiterleitung der ausgewerteten EEP-Daten an MQTT.
    }

    // Funktion zum Starten des Anlernmodus über ein MQTT/YAML-Event
    void start_pairing() {
        this->pairing_active_ = true;
        ESP_LOGI(TAG, "Anlernmodus aktiviert!");
    }

    // Funktion zum Stoppen des Anlernmodus
    void stop_pairing() {
        this->pairing_active_ = false;
        ESP_LOGI(TAG, "Anlernmodus beendet.");
    }

    // NEU: Speichert ein dynamisch angelerntes Gerät dauerhaft ab
    void save_learned_device(uint32_t device_id, const std::string &name, const std::string &eep) {
        // Prüfen, ob das Gerät bereits existiert
        for (const auto &dev : this->nvs_devices_) {
            if (dev.device_id == device_id) {
                ESP_LOGI(TAG, "Gerät %08X ist bereits im NVS bekannt.", device_id);
                return;
            }
        }

        // Prüfen, ob das konfigurierte Limit erreicht ist
        if (this->nvs_devices_.size() >= this->max_dev_) {
            ESP_LOGE(TAG, "Speicher voll! Maximal %u Geräte erlaubt.", this->max_dev_);
            return;
        }

        // Neues Gerät erstellen und befüllen
        SavedDevice new_dev;
        new_dev.device_id = device_id;
        memset(new_dev.name, 0, sizeof(new_dev.name));
        memset(new_dev.eep, 0, sizeof(new_dev.eep));
        strncpy(new_dev.name, name.c_str(), sizeof(new_dev.name) - 1);
        strncpy(new_dev.eep, eep.c_str(), sizeof(new_dev.eep) - 1);

        // Im Vektor und anschließend im NVS-Flash speichern
        this->nvs_devices_.push_back(new_dev);
        save_to_nvs();
        ESP_LOGI(TAG, "Gerät %08X (%s, %s) erfolgreich im NVS gelernt.", device_id, name.c_str(), eep.c_str());
    }

protected:
    bool parse_all_dev_{false};
    uint32_t max_dev_{20}; // Standardwert, wird von Python überschrieben (20 für ESP8266, 40 für ESP32)
    bool pairing_active_{false};

    // Speicherverwaltung
    ESPPreferenceObject pref_;
    std::vector<SavedDevice> nvs_devices_;        // Aktive Liste aller bekannten Geräte (YAML + gelernt)
    std::vector<YamlDevice> yaml_devices_;        // Temporäre Liste der beim Booten übergebenen YAML-Geräte

    // Lädt alle gespeicherten Geräte aus dem Flash-Speicher
    void load_from_nvs() {
        size_t pref_size = sizeof(size_t) + (this->max_dev_ * sizeof(SavedDevice));
        std::vector<uint8_t> buffer(pref_size, 0);

        if (this->pref_.load(buffer.data())) {
            size_t device_count = 0;
            // Die ersten Bytes enthalten die Anzahl der aktuell gespeicherten Geräte
            memcpy(&device_count, buffer.data(), sizeof(size_t));

            // Sicherheitsprüfung, um Speicherüberläufe zu verhindern
            if (device_count > this->max_dev_) {
                device_count = this->max_dev_;
            }

            this->nvs_devices_.clear();
            size_t offset = sizeof(size_t);
            for (size_t i = 0; i < device_count; i++) {
                SavedDevice dev;
                memcpy(&dev, buffer.data() + offset, sizeof(SavedDevice));
                this->nvs_devices_.push_back(dev);
                offset += sizeof(SavedDevice);
            }
            ESP_LOGI(TAG, "%u Geräte erfolgreich aus dem NVS geladen.", device_count);
        } else {
            ESP_LOGI(TAG, "Keine gespeicherten Geräte im NVS gefunden (Erster Start).");
        }
    }

    // Schreibt den aktuellen nvs_devices_ Vektor zurück in den NVS-Flash
    void save_to_nvs() {
        size_t pref_size = sizeof(size_t) + (this->max_dev_ * sizeof(SavedDevice));
        std::vector<uint8_t> buffer(pref_size, 0);

        size_t current_count = this->nvs_devices_.size();
        // 1. Anzahl der Geräte an den Anfang schreiben
        memcpy(buffer.data(), &current_count, sizeof(size_t));

        // 2. Geräte-Strukturen dahinter anfügen
        size_t offset = sizeof(size_t);
        for (const auto &dev : this->nvs_devices_) {
            memcpy(buffer.data() + offset, &dev, sizeof(SavedDevice));
            offset += sizeof(SavedDevice);
        }

        // 3. Im Flash persistieren
        if (this->pref_.save(buffer.data())) {
            ESP_LOGD(TAG, "NVS-Speicher erfolgreich aktualisiert (%u Geräte gesichert).", current_count);
        } else {
            ESP_LOGE(TAG, "Fehler beim Schreiben in den NVS-Speicher!");
        }
    }

    // NEU: Abgleich der YAML-Geräte mit dem NVS-Speicher beim Booten
    void sync_yaml_to_nvs() {
        bool need_save = false;

        for (const auto &yaml_dev : this->yaml_devices_) {
            bool found = false;
            // Prüfen, ob das YAML-Gerät bereits im NVS ist
            for (const auto &nvs_dev : this->nvs_devices_) {
                if (nvs_dev.device_id == yaml_dev.device_id) {
                    found = true;
                    break;
                }
            }

            // Wenn das YAML-Gerät noch nicht im NVS existiert, fügen wir es hinzu
            if (!found) {
                if (this->nvs_devices_.size() < this->max_dev_) {
                    SavedDevice new_dev;
                    new_dev.device_id = yaml_dev.device_id;
                    memset(new_dev.name, 0, sizeof(new_dev.name));
                    memset(new_dev.eep, 0, sizeof(new_dev.eep));
                    strncpy(new_dev.name, yaml_dev.name.c_str(), sizeof(new_dev.name) - 1);
                    strncpy(new_dev.eep, yaml_dev.eep.c_str(), sizeof(new_dev.eep) - 1);

                    this->nvs_devices_.push_back(new_dev);
                    need_save = true;
                    ESP_LOGI(TAG, "YAML-Gerät %08X (%s) neu in den NVS synchronisiert.", yaml_dev.device_id, yaml_dev.name.c_str());
                } else {
                    ESP_LOGE(TAG, "NVS voll! YAML-Gerät %08X konnte nicht synchronisiert werden.", yaml_dev.device_id);
                }
            }
        }

        // Falls neue YAML-Geräte hinzugefügt wurden, schreiben wir die Daten einmalig zurück
        if (need_save) {
            save_to_nvs();
        }
    }
};

} // namespace enocean
} // namespace esphome
