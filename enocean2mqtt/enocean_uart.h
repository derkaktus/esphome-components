#pragma once
#include <vector>
#include <queue>
#include <string>
#include <cstdint>

namespace esphome {
namespace enocean_mqtt {

// ─────────────────────────────────────────────────────────────────────────────
// EnOcean ESP3 Telegramm Struktur
// ─────────────────────────────────────────────────────────────────────────────

struct EnoceanTelegram {
    uint8_t  rorg       {0x00};         // Packet Type / RORG
    uint8_t  data[32]   {};             // Nutzdaten (max 32 Bytes)
    uint8_t  data_len   {0};            // Länge Nutzdaten
    std::string sender_id;              // Sender ID (8 Hex Zeichen, z.B. "AABBCCDD")
    uint8_t  status     {0x00};         // Status Byte
    int      rssi       {0};            // RSSI (aus Optional Data)
    bool     valid      {false};        // CRC ok?
};

// ─────────────────────────────────────────────────────────────────────────────
// EnOcean ESP3 Parser
// ─────────────────────────────────────────────────────────────────────────────

class EnoceanUartParser {
public:

    // Byte einlesen
    void feed(uint8_t byte);

    // Liegt ein vollständiges Telegramm vor?
    bool has_telegram() const { return !telegram_queue_.empty(); }

    // Nächstes Telegramm holen (und aus Queue entfernen)
    EnoceanTelegram pop_telegram();

private:

    // ── ESP3 Frame States ─────────────────────────────────────
    enum class State {
        WAIT_SYNC,          // Warte auf 0x55
        HEADER_1,           // Data Length High
        HEADER_2,           // Data Length Low
        HEADER_3,           // Optional Length
        HEADER_4,           // Packet Type
        CRC8H,              // Header CRC
        DATA,               // Datenbytes
        OPTIONAL,           // Optional Datenbytes
        CRC8D               // Data CRC
    };

    State    state_          {State::WAIT_SYNC};

    uint16_t data_length_    {0};
    uint8_t  opt_length_     {0};
    uint8_t  packet_type_    {0};
    uint8_t  header_crc_     {0};

    std::vector<uint8_t> data_buf_;
    std::vector<uint8_t> opt_buf_;

    uint16_t byte_count_     {0};

    // ── CRC Berechnung ────────────────────────────────────────
    uint8_t crc8(const uint8_t* data, size_t len);
    uint8_t crc8_byte(uint8_t crc, uint8_t byte);

    // ── Telegramm zusammenbauen ───────────────────────────────
    void process_frame();

    // ── Fertige Telegramme ────────────────────────────────────
    std::queue<EnoceanTelegram> telegram_queue_;

    // ── Hilfsfunktionen ───────────────────────────────────────
    std::string bytes_to_id(const uint8_t* bytes, int len);
};

} // namespace enocean_mqtt
} // namespace esphome
