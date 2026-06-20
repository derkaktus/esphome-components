# enocean_hub
Dient dazu, EnOcean-Funktelegramme über UART entgegen zu nehmen
(ESP3-Protokoll, z. B. TCM310 / USB300-Modul), zu parsen und als
ESPHome-Sensoren bereitzustellen.

### Features
- Unterstützt Geräte nach EEP F6_02_01
- Unterstützt Geräte nach EEP F6_10_00
- Debug Sensor zur identifikation neuer bzw. unbekannter Geräte
- Darstellung von EnOcean Geräten als Binary-Sensor
- Darstellung von EnOcean Geräten als Text-Sensor

### Funktionsweise:
1. `EnoceanHub` liest UART-Bytes und parst sie als ESP3-Telegramme
   (Sync-Byte `0x55`, Header mit CRC8, Daten mit CRC8).
2. Bei einem gültigen Funktelegramm (Packet Type `RADIO_ERP1`) wird die
   Sender-ID extrahiert und an alle registrierten `EnoceanDevice`-Objekte
   mit passender Sender-ID weitergereicht.
3. Jedes `EnoceanDevice` besitzt ein EEP-Parser-Objekt (`EepBase`-Ableitung),
   das die Rohdaten gemäß EEP-Spezifikation interpretiert und die
   entsprechenden Sensoren (`binary_sensor`, `sensor`, `text_sensor`)
   publiziert.
4. Ist `debug_dev: true` gesetzt, gibt ein zusätzlicher `text_sensor`
   **jedes** empfangene Telegramm aus – auch von nicht registrierten
   (unbekannten) Geräten. Praktisch, um neue Sender-IDs herauszufinden.


### Einbindung der Komponente in esphome Projekte
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/derkaktus/esphome-components
    components: [ enocean_hub ]
```

**Konfiguration des Hub:**
```yaml
uart:
  id: enocean_uart
  tx_pin: GPIOXX
  rx_pin: GPIOXX
  baud_rate: 57600

enocean_hub:
  id: my_enocean_hub
  uart_id: enocean_uart
  debug_dev: true
  debug_sensor:
    name: "EnOcean Debug Sensor"
```

| Option          | Pflicht | Beschreibung                                          |
|-----------------|---------|--------------------------------------------------------|
| `uart_id`       | ja      | UART-Bus zum EnOcean-Transceiver                        |
| `debug_dev`     | nein    | `true` aktiviert die Debug-Ausgabe aller Telegramme      |
| `debug_sensor`  | nein    | Text-Sensor-Konfiguration für die Debug-Ausgabe (erfordert `debug_dev: true`) |

Konfiguration der Sensoren:
**EEP F6-02-01 (Wippschalter, 2-Wippe)**

```yaml
binary_sensor:
  - platform: enocean_hub
    enocean_hub_id: my_enocean_hub
    sender_id: 0x0123ABCD
    eep: F6-02-01
    button_a_top:
      name: "Wippe A oben"
    button_a_bottom:
      name: "Wippe A unten"
    button_b_top:
      name: "Wippe B oben"
    button_b_bottom:
      name: "Wippe B unten"

text_sensor:
  - platform: enocean_hub
    enocean_hub_id: my_enocean_hub
    sender_id: 0x0123ABCD
    eep: F6-02-01
    action:
      name: "Wippschalter letzte Aktion"
```
**EEP F6-10-00 (Fenstergriff)**

```yaml
sensor:
  - platform: enocean_hub
    enocean_hub_id: my_enocean_hub
    sender_id: 0x0456EF01
    eep: F6-10-00
    position:
      name: "Fenstergriff Position (numerisch)"
      # 0 = geschlossen, 1 = gekippt, 2 = offen

text_sensor:
  - platform: enocean_hub
    enocean_hub_id: my_enocean_hub
    sender_id: 0x0456EF01
    eep: F6-10-00
    position_text:
      name: "Fenstergriff Position"
      # "closed" | "tilted" | "open" | "unknown"
```

Anmerkung:
Die genauen Datenbyte-Werte für F6-10-00 (Fenstergriff) variieren je nach 
Hersteller. Die in `eep_F6_10_00.cpp` hinterlegten Werte (`0xF0`/`0xE0`
geschlossen, `0xD0` gekippt, `0xC0` offen) sind die gebräuchlichsten;
bei Bedarf anhand der `debug_dev`-Ausgabe für dein konkretes Gerät anpassen.

### Neue EEPs hinzufügen

1. Neue Datei `eep/eep_XX_XX_XX.h` + `.cpp` anlegen, von `EepBase` ableiten.
2. `parse()` implementieren (RORG prüfen, Datenbytes gemäß EEP-Spec dekodieren,
   Sensoren publizieren).
3. Include in `enocean_device.h` ergänzen.
4. In den passenden `binary_sensor/__init__.py`, `sensor/__init__.py` bzw.
   `text_sensor/__init__.py` das neue EEP-Kürzel zu `SUPPORTED_EEPS` hinzufügen
   und im `to_code()` einen neuen `elif eep == "XX-XX-XX":`-Zweig ergänzen,
   der das passende C++-Objekt erzeugt und die Sensoren verdrahtet.
