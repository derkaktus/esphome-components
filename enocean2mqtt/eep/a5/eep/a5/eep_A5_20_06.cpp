#include "eep_A5_20_06.h"

namespace esphome {
namespace enocean_mqtt {

std::vector<MqttMessage> EepA5_20_06::parse(
    const uint8_t* data,
    uint8_t data_len,
    const std::string& base_topic
) {
    std::vector<MqttMessage> msgs;
    if (data_len < 4) return msgs;

    // ── DB3: Valve Position ──────────────────────────────────
    // 0x00 = 0% (geschlossen), 0xFF = 100% (offen)
    uint8_t raw_valve = data[0];
    int valve_position = (int)((raw_valve / 255.0f) * 100.0f);

    // ── DB2: Temperature ─────────────────────────────────────
    // Skalierung: 0x00 = 0°C, 0xFF = 40°C
    uint8_t raw_temp = data[1];
    float temperature = (raw_temp / 255.0f) * 40.0f;

    // ── DB1: Setpoint / Offset ────────────────────────────────
    // LOM (DB0.Bit1) bestimmt Interpretation:
    // LOM=1 → Setpoint Temperatur (0°C - 40°C)
    // LOM=0 → Temperatur Offset   (-5°C - +5°C, Mitte = 0x80)
    uint8_t raw_db1 = data[2];

    // ── DB0: Status Bits ──────────────────────────────────────
    uint8_t db0 = data[3];

    bool harvesting  = (db0 & 0x80) != 0;  // Bit7: Energy Harvesting aktiv
    bool energy_ok   = (db0 & 0x40) != 0;  // Bit6: Energie ausreichend
    bool tsl         = (db0 & 0x20) != 0;  // Bit5: Temperature Sensor Location
                                             //       0=ambient, 1=feed
    bool lom         = (db0 & 0x02) != 0;  // Bit1: Local Operation Mode
                                             //       0=offset, 1=setpoint
    bool window_open = (db0 & 0x10) != 0;  // Bit4: Fensterkontakt offen
    bool radio_error = (db0 & 0x08) != 0;  // Bit3: Radio Fehler
    bool signal_weak = (db0 & 0x04) != 0;  // Bit2: Signal schwach
    bool obstructed  = (db0 & 0x01) != 0;  // Bit0: Ventil blockiert

    // Temperatur Sensorquelle
    std::string temp_sensor = tsl ? "feed" : "ambient";

    // Setpoint / Offset auflösen
    std::string setpoint_str;
    std::string offset_str;

    if (lom) {
        // Setpoint Modus: 0x00 = 0°C, 0xFF = 40°C
        float setpoint = (raw_db1 / 255.0f) * 40.0f;
        setpoint_str = float_to_str(setpoint);
        offset_str   = "0.0";
    } else {
        // Offset Modus: 0x00 = -5°C, 0x80 = 0°C, 0xFF = +5°C
        float offset = ((raw_db1 / 255.0f) * 10.0f) - 5.0f;
        setpoint_str = "0.0";
        offset_str   = float_to_str(offset);
    }

    // ── MQTT Topics ───────────────────────────────────────────
    add_msg(msgs, base_topic, "valve_position", int_to_str(valve_position));
    add_msg(msgs, base_topic, "temperature",    float_to_str(temperature));
    add_msg(msgs, base_topic, "temp_sensor",    temp_sensor);
    add_msg(msgs, base_topic, "setpoint",       setpoint_str);
    add_msg(msgs, base_topic, "temp_offset",    offset_str);
    add_msg(msgs, base_topic, "harvesting",     bool_to_str(harvesting));
    add_msg(msgs, base_topic, "energy_ok",      bool_to_str(energy_ok));
    add_msg(msgs, base_topic, "window_open",    bool_to_str(window_open));
    add_msg(msgs, base_topic, "radio_error",    bool_to_str(radio_error));
    add_msg(msgs, base_topic, "signal_weak",    bool_to_str(signal_weak));
    add_msg(msgs, base_topic, "obstructed",     bool_to_str(obstructed));

    // ── State JSON (Zusammenfassung) ──────────────────────────
    auto json = build_json({
        {"valve_position", int_to_str(valve_position)},
        {"temperature",    float_to_str(temperature)},
        {"temp_sensor",    json_str(temp_sensor)},
        {"setpoint",       setpoint_str},
        {"temp_offset",    offset_str},
        {"lom",            bool_to_str(lom)},
        {"harvesting",     bool_to_str(harvesting)},
        {"energy_ok",      bool_to_str(energy_ok)},
        {"window_open",    bool_to_str(window_open)},
        {"radio_error",    bool_to_str(radio_error)},
        {"signal_weak",    bool_to_str(signal_weak)},
        {"obstructed",     bool_to_str(obstructed)}
    });

    add_msg(msgs, base_topic, "state", json);

    return msgs;
}

} // namespace enocean_mqtt
} // namespace esphome
