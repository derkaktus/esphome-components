#include "esphome/components/enocean_mqtt/enocean_uart.h"
#include "esphome/core/log.h"
#include <cstdio>

namespace esphome {
namespace enocean_mqtt {

static const char* TAG = "enocean_uart";

// ESP3 Sync Byte
static const uint8_t SYNC_BYTE    = 0x55;

// ESP3 Packet Type für Radio ERP1
static const uint8_t PKT_RADIO_ERP1 = 0x01;

// ─────────────────────────────────────────────────────────────────────────────
// Byte einlesen & State Machine
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanUartParser::feed(uint8_t byte) {

    switch (state_) {

        // ── Warte auf Sync Byte 0x55 ──────────────────────────
        case State::WAIT_SYNC:
            if (byte == SYNC_BYTE) {
                data_buf_.clear();
                opt_buf_.clear();
                byte_count_ = 0;
                state_ = State::HEADER_1;
            }
            break;

        // ── Header: Data Length High ──────────────────────────
        case State::HEADER_1:
            data_length_ = ((uint16_t)byte) << 8;
            state_ = State::HEADER_2;
            break;

        // ── Header: Data Length Low ───────────────────────────
        case State::HEADER_2:
            data_length_ |= byte;
            state_ = State::HEADER_3;
            break;

        // ── Header: Optional Length ───────────────────────────
        case State::HEADER_3:
            opt_length_ = byte;
            state_ = State::HEADER_4;
            break;

        // ── Header: Packet Type ───────────────────────────────
        case State::HEADER_4:
            packet_type_ = byte;
            state_ = State::CRC8H;
            break;

        // ── Header CRC prüfen ─────────────────────────────────
        case State::CRC8H: {
            // Header CRC über 4 Bytes: DataLen(2), OptLen, PktType
            uint8_t hdr[4] = {
                (uint8_t)(data_length_ >> 8),
                (uint8_t)(data_length_ & 0xFF),
                opt_length_,
                packet_type_
            };
            uint8_t calc = crc8(hdr, 4);

            if (calc != byte) {
                ESP_LOGW(TAG, "Header CRC mismatch: calc=0x%02X recv=0x%02X",
                    calc, byte);
                state_ = State::WAIT_SYNC;
            } else {
                data_buf_.reserve(data_length_);
                opt_buf_.reserve(opt_length_);
                byte_count_ = 0;
                state_ = (data_length_ > 0) ? State::DATA : State::OPTIONAL;
            }
            break;
        }

        // ── Datenbytes einlesen ───────────────────────────────
        case State::DATA:
            data_buf_.push_back(byte);
            byte_count_++;

            if (byte_count_ >= data_length_) {
                byte_count_ = 0;
                state_ = (opt_length_ > 0) ? State::OPTIONAL : State::CRC8D;
            }
            break;

        // ── Optional Bytes einlesen ───────────────────────────
        case State::OPTIONAL:
            opt_buf_.push_back(byte);
            byte_count_++;

            if (byte_count_ >= opt_length_) {
                byte_count_ = 0;
                state_ = State::CRC8D;
            }
            break;

        // ── Data CRC prüfen & Frame verarbeiten ───────────────
        case State::CRC8D: {
            // CRC über Data + Optional
            uint8_t calc = crc8(data_buf_.data(), data_buf_.size());
            // Optional Bytes in CRC mit einrechnen
            for (auto b : opt_buf_) {
                calc = crc8_byte(calc, b);
            }

            if (calc != byte) {
                ESP_LOGW(TAG, "Data CRC mismatch: calc=0x%02X recv=0x%02X",
                    calc, byte);
            } else {
                process_frame();
            }
            state_ = State::WAIT_SYNC;
            break;
        }

        default:
            state_ = State::WAIT_SYNC;
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame verarbeiten → EnoceanTelegram aufbauen
// ─────────────────────────────────────────────────────────────────────────────

void EnoceanUartParser::process_frame() {

    // Nur Radio ERP1 Pakete verarbeiten
    if (packet_type_ != PKT_RADIO_ERP1) {
        ESP_LOGD(TAG, "Ignoring packet type: 0x%02X", packet_type_);
        return;
    }

    // Mindestlänge prüfen: RORG(1) + Data + SenderID(4) + Status(1)
    if (data_buf_.size() < 6) {
        ESP_LOGW(TAG, "Frame too short: %d bytes", (int)data_buf_.size());
        return;
    }

    EnoceanTelegram tg;
    tg.valid = true;

    // RORG (erstes Byte der Daten)
    tg.rorg = data_buf_[0];

    // Nutzdaten: nach RORG, vor SenderID(4) + Status(1)
    int payload_len = (int)data_buf_.size() - 5; // -1 RORG -4 ID -1 Status + 1 RORG?
    // Korrekte Berechnung:
    // data_buf_ = [RORG][payload...][SenderID 4 bytes][Status]
    // payload_len = data_buf_.size() - 1(RORG) - 4(ID) - 1(Status)

    payload_len = (int)data_buf_.size() - 6;

    if (payload_len < 0) payload_len = 0;
    if (payload_len > 32) payload_len = 32;

    tg.data_len = (uint8_t)payload_len;

    for (int i = 0; i < payload_len; i++) {
        tg.data[i] = data_buf_[1 + i];
    }

    // Sender ID (4 Bytes vor Status)
    int id_offset = (int)data_buf_.size() - 5;
    if (id_offset >= 1) {
        tg.sender_id = bytes_to_id(
            data_buf_.data() + id_offset,
            4
        );
    }

    // Status (letztes Byte)
    tg.status = data_buf_.back();

    // RSSI aus Optional Data (Byte 1 wenn vorhanden)
    if (opt_buf_.size() >= 2) {
        // Optional[0] = SubTelNum
        // Optional[1] = Destination ID (4 bytes)
        // Optional[5] = RSSI
        if (opt_buf_.size() >= 6) {
            tg.rssi = -(int)opt_buf_[5]; // RSSI ist negativ
        }
    }

    ESP_LOGD(TAG, "Frame OK: RORG=0x%02X sender=%s payload=%d bytes",
        tg.rorg,
        tg.sender_id.c_str(),
        tg.data_len);

    telegram_queue_.push(tg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Nächstes Telegramm holen
// ─────────────────────────────────────────────────────────────────────────────

EnoceanTelegram EnoceanUartParser::pop_telegram() {
    auto tg = telegram_queue_.front();
    telegram_queue_.pop();
    return tg;
}

// ─────────────────────────────────────────────────────────────────────────────
// CRC8 (EnOcean Variante: Polynom 0x07)
// ─────────────────────────────────────────────────────────────────────────────

uint8_t EnoceanUartParser::crc8_byte(uint8_t crc, uint8_t byte) {
    static const uint8_t CRC8_TABLE[256] = {
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
        0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
        0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
        0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
        0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
        0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
        0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
        0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
        0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
        0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
        0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
        0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
        0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
        0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
        0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
        0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
        0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
        0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
        0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
        0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
        0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
        0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
        0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
        0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
        0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
        0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
        0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
    };
    return CRC8_TABLE[crc ^ byte];
}

uint8_t EnoceanUartParser::crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc = crc8_byte(crc, data[i]);
    }
    return crc;
}

// ─────────────────────────────────────────────────────────────────────────────
// Hilfsfunktion: Bytes → Hex String ID
// ─────────────────────────────────────────────────────────────────────────────

std::string EnoceanUartParser::bytes_to_id(const uint8_t* bytes, int len) {
    char buf[16] = {0};
    if (len == 4) {
        snprintf(buf, sizeof(buf), "%02X%02X%02X%02X",
            bytes[0], bytes[1], bytes[2], bytes[3]);
    }
    return std::string(buf);
}

} // namespace enocean_mqtt
} // namespace esphome
