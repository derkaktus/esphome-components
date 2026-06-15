#pragma once

#include "esphome.h"
#include "eep_parser.h"
#include <vector>
#include <algorithm>

static const char *const TAG = "enocean_bridge";

// Struktur für persistente Speicherung im NVS-Flash
struct SavedDevice {
    uint32_t device_id;
    char name[32];
    char eep[16];
};

class EnOceanBridge : public Component, public uart::UARTDevice {
 protected:
    ESPPreferenceObject pref_;
    std::vector<SavedDevice> nvs_devices_;
    std::vector<uint8_t> rx_buffer_;
    bool parse_all_dev_{false};
    size_t max_dev_{40};
    
    // Pairing-Variablen
    bool pairing_mode_{false};
    uint32_t pairing_start_time_{0};
    const uint32_t pairing_timeout_{60000}; // 60 Sekunden Timeout

    // Sicherung der statischen YAML-Geräte im RAM
    std::vector<SavedDevice> yaml_devices_;
    bool nvs_ready_{false};

 public:
    EnOceanBridge() = default;

    void set_parse_all_dev(bool val) { this->parse_all_dev_ = val; }
    void set_max_dev(size_t val) { this->max_dev_ = val; }

    // Registriert Geräte, die fest im YAML eingetragen sind
    void add_yaml_device(uint32_t device_id, const std::string& name, const std::string& eep) {
        SavedDevice dev{};
        dev.device_id = device_id;
        strncpy(dev.name, name.c_str(), sizeof(dev.name) - 1);
        strncpy(dev.eep, eep.c_str(), sizeof(dev.eep) - 1);
        this->yaml_devices_.push_back(dev);
    }

    // Startet den Anlernmodus (kann über MQTT-Trigger oder Yaml-Button aufgerufen werden)
    void start_pairing(bool enable) {
        if (enable) {
            this->pairing_mode_ = true;
            this->pairing_start_time_ = millis();
            ESP_LOGI(TAG, "Anlernmodus (Pairing) GESTARTET für 60 Sekunden.");
        } else {
            this->pairing_mode_ = false;
            ESP_LOGI(TAG, "Anlernmodus (Pairing) BEENDET.");
        }
    }

    void setup() override {
        ESP_LOGI(TAG, "Initialisiere EnOcean Bridge...");
        this->set_rx_full_threshold(256); 

        // 1. Initialisiere die aktive Liste im RAM sofort mit den YAML-Geräten
        this->nvs_devices_ = this->yaml_devices_;

        // 2. NVS-Speicher laden mit Absicherung gegen Abstürze
        try {
            size_t pref_size = sizeof(SavedDevice) * this->max_dev_;
            this->pref_ = global_preferences->make_preference(pref_size, fnv1_hash("enocean_devices"));

            std::vector<SavedDevice> temp_buffer(this->max_dev_);
            if (this->pref_.load(temp_buffer.data())) {
                ESP_LOGI(TAG, "NVS-Daten erfolgreich aus Flash geladen.");
                this->nvs_ready_ = true;
                
                // Füge geladene NVS-Geräte hinzu, sofern sie noch nicht im YAML definiert waren
                for (const auto& dev : temp_buffer) {
                    if (dev.device_id != 0) {
                        auto it = std::find_if(this->nvs_devices_.begin(), this->nvs_devices_.end(),
                            [&dev](const SavedDevice& d) { return d.device_id == dev.device_id; });
                        
                        if (it == this->nvs_devices_.end()) {
                            this->nvs_devices_.push_back(dev);
                            ESP_LOGI(TAG, "Gerät aus Flash geladen: 0x%08X (%s) [%s]", dev.device_id, dev.name, dev.eep);
                        }
                    }
                }
            } else {
                ESP_LOGW(TAG, "NVS-Speicher ist leer oder uninitialisiert. Verwende YAML-Geräte.");
                this->nvs_ready_ = true;
                this->save_to_flash(); // Schreibt den aktuellen Zustand (YAML) initial in das NVS
            }
        } catch (...) {
            ESP_LOGE(TAG, "Kritischer Fehler bei der NVS-Initialisierung! Notfall-Fallback auf YAML-only.");
            this->nvs_ready_ = false;
        }

        ESP_LOGI(TAG, "Bridge gestartet. %d Geräte aktiv registriert.", this->nvs_devices_.size());
    }

    void loop() override {
        // Kontinuierlich eingehende Bytes vom UART lesen
        while (this->available() > 0) {
            uint8_t byte = this->read();
            this->rx_buffer_.push_back(byte);
        }

        if (!this->rx_buffer_.empty()) {
            this->process_buffer();
        }

        // Timeout für Anlernmodus prüfen
        if (this->pairing_mode_ && (millis() - this->pairing_start_time_ > this->pairing_timeout_)) {
            ESP_LOGI(TAG, "Anlernmodus beendet (Timeout abgelaufen).");
            this->pairing_mode_ = false;
        }
    }

 protected:
    void save_to_flash() {
        if (!this->nvs_ready_) return;

        std::vector<SavedDevice> temp_buffer(this->max_dev_, SavedDevice{0, "", ""});
        for (size_t i = 0; i < this->nvs_devices_.size() && i < this->max_dev_; ++i) {
            temp_buffer[i] = this->nvs_devices_[i];
        }
        
        if (this->pref_.save(temp_buffer.data())) {
            global_preferences->sync();
            ESP_LOGD(TAG, "Geräteliste im Flash-Speicher gesichert.");
        } else {
            ESP_LOGW(TAG, "Speichern der Geräteliste im Flash-Speicher fehlgeschlagen.");
        }
    }

    void process_buffer() {
        // Suche nach dem EnOcean ESP3 Sync-Byte (0x55)
        while (!this->rx_buffer_.empty() && this->rx_buffer_.front() != 0x55) {
            this->rx_buffer_.erase(this->rx_buffer_.begin());
        }

        if (this->rx_buffer_.size() < 6) return; // Header ist noch unvollständig

        uint16_t data_len = (this->rx_buffer_[1] << 8) | this->rx_buffer_[2];
        uint8_t opt_len = this->rx_buffer_[3];
        uint8_t packet_type = this->rx_buffer_[4];

        size_t total_packet_len = 6 + data_len + opt_len + 1; // 6 Header + Data + Opt + 1 CRC

        if (this->rx_buffer_.size() < total_packet_len) return; // Paket noch nicht vollständig übertragen

        // Vollständiges Paket aus dem Puffer kopieren und im Puffer löschen
        std::vector<uint8_t> packet(this->rx_buffer_.begin(), this->rx_buffer_.begin() + total_packet_len);
        this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + total_packet_len);

        this->parse_esp3_packet(packet);
    }

    void parse_esp3_packet(const std::vector<uint8_t>& packet) {
        uint16_t data_len = (packet[1] << 8) | packet[2];
        uint8_t opt_len = packet[3];
        uint8_t packet_type = packet[4];

        // Wir interessieren uns nur für Radio Erp1 Telegramme (Typ 0x01)
        if (packet_type != 0x01) return;

        // payload beginnt bei Index 6
        size_t payload_offset = 6;
        if (data_len < 6) return; // Zu kurzes Radio-Telegramm

        uint8_t rorg = packet[payload_offset];
        
        // Sender-ID liegt am Ende der Nutzdaten (vor den optionalen Daten)
        // Bei ERP1 ist die Sender-ID immer 4 Bytes vor dem Ende der Nutzdaten
        size_t id_offset = payload_offset + data_len - 5; 
        uint32_t sender_id = ((uint32_t)packet[id_offset] << 24) |
                             ((uint32_t)packet[id_offset + 1] << 16) |
                             ((uint32_t)packet[id_offset + 2] << 8) |
                             (uint32_t)packet[id_offset + 3];

        ESP_LOGD(TAG, "Paket empfangen: RORG=0x%02X, Sender-ID=0x%08X", rorg, sender_id);

        // Überprüfen, ob das Gerät registriert ist
        auto it = std::find_if(this->nvs_devices_.begin(), this->nvs_devices_.end(),
            [sender_id](const SavedDevice& dev) { return dev.device_id == sender_id; });

        bool is_registered = (it != this->nvs_devices_.end());

        // 1. FALLS PAIRING AKTIV IST: Neues Gerät anlernen (A5-06-10-00 Teach-In Telegramm)
        if (this->pairing_mode_ && !is_registered) {
            // Ein Teach-In bei 4BS (RORG 0xA5) hat oft das 3. Byte (LRN-Bit) auf 0 gesetzt
            // Für EEP A5-06-10-00 prüfen wir das Vorhandensein
            if (rorg == 0xA5) {
                uint8_t db0 = packet[payload_offset + data_len - 6]; // DB0 Byte
                bool lrn_bit = !(db0 & 0x08); // LRN-Bit ist invertiert (0 = Teach-in, 1 = Data)

                if (lrn_bit) {
                    ESP_LOGI(TAG, "Teach-In Telegramm von neuem Gerät empfangen! ID: 0x%08X", sender_id);
                    
                    if (this->nvs_devices_.size() < this->max_dev_) {
                        SavedDevice new_dev{};
                        new_dev.device_id = sender_id;
                        
                        // Generiere einen sprechenden Namen
                        snprintf(new_dev.name, sizeof(new_dev.name), "learned_0x%08X", sender_id);
                        
                        // Für das angefragte Gerät setzen wir das EEP auf A5-06-10-00
                        strncpy(new_dev.eep, "A5-06-10-00", sizeof(new_dev.eep) - 1);

                        this->nvs_devices_.push_back(new_dev);
                        this->save_to_flash(); // Sofort im NVS-Flash sichern

                        ESP_LOGI(TAG, "Gerät erfolgreich gelernt und gespeichert: %s", new_dev.name);
                        
                        // Pairing-Modus nach erfolgreichem Anlernen beenden
                        this->pairing_mode_ = false;
                        return;
                    } else {
                        ESP_LOGE(TAG, "Geräteliste voll! Maximal %d Geräte erlaubt.", this->max_dev_);
                    }
                }
            }
        }

        // 2. TELEGRAMM VERARBEITEN: Wenn das Gerät registriert ist oder "parse_all" aktiv ist
        if (is_registered || this->parse_all_dev_) {
            std::string device_name = is_registered ? it->name : "Unknown";
            std::string eep = is_registered ? it->eep : "";

            // Falls Gerät unbekannt, versuchen wir, anhand des RORG ein Standard-EEP zu raten
            if (eep.empty()) {
                if (rorg == 0xF6) eep = "F6-10-00";
                else if (rorg == 0xA5) eep = "A5-02-05"; // Default 4BS
            }

            ESP_LOGI(TAG, "Verarbeite Telegramm für Gerät: %s (ID: 0x%08X) via EEP: %s", 
                     device_name.c_str(), sender_id, eep.c_str());

            // Nutzdaten extrahieren (ohne RORG, ID und Status)
            size_t data_bytes_len = data_len - 6; // payload minus RORG(1), Sender-ID(4), Status(1)
            std::vector<uint8_t> data_bytes(packet.begin() + payload_offset + 1, 
                                            packet.begin() + payload_offset + 1 + data_bytes_len);

            // Paket an die `eep_parser.h` übergeben
            parse_and_publish(eep, data_bytes, sender_id, device_name);
        }
    }
};
