Enocean-2-MQTT
**enocean2mqtt** ist ein ESPHome Custom Component, das EnOcean-Funk Telegramme über UART empfängt und diese als JSON-Nachrichten über MQTT veröffentlicht. Das Component unterstützt eine Vielzahl von EEPs (EnOcean Equipment Profiles) und ermöglicht die dynamische Verwaltung von Geräten zur Laufzeit.

Das Component fungiert als **Bridge** zwischen der EnOcean-Welt und dem MQTT-basierten Smart Home. Es ist vollständig in ESPHome integrierbar.

## Unterstützte Geräteklassen

|  EEP  |  Datei | Beschreibung   |
| ------------ | ------------ | ------------ |
| F6-02-01  | f6/eep_F6_02_01 |  Rocker Switch  |
| F6-10-00  | f6/eep_F6_10_00  | Window Handle  |
| D5-00-01  | d5/eep_D5_00_01  | Contact Sensor  |
| A5-02-05  | a5/eep_A5_02_05  | Temperature  |
| A5-06-02  | a5/eep_A5_06_02  | Light Sensor  |
| A5-07-01  | a5/eep_A5_07_01  |  Occupancy  |
| A5-09-04  | a5/eep_A5_09_04  | CO2/Hum/Temp  |
| A5-10-06  | a5/eep_A5_10_06  |  Room Panel |
| A5-20-06  | a5/eep_A5_20_06  |  HVAC Harvesting |

## Features

| Feature | Beschreibung |
|---------|--------------|
| **Vollständige ESP3-Parsierung** | UART-basierter ESP3-Frame-Parser mit CRC8-Validierung |
| **EEP-Unterstützung** | Unterstützung für RPS, 1BS, 4BS Telegramme |
| **Modulare Architektur** | Jeder EEP-Parser in separater Datei, einfach erweiterbar |
| **Runtime Device Management** | Geräte per MQTT hinzufügen, entfernen, auflisten |
| **MQTT-Integration** | JSON-basierte Nachrichten, retain-Funktion |
| **Debug-Output** | Optionale Raw-Telegramm-Ausgabe |
| **Known Devices** | Statische Gerätekonfiguration in YAML |
| **Automatische Erkennung** | Teach-In-Unterstützung für neue Geräte |


### Getestete Hardware
- ** TCM300**
- ** TCM310**


## Beispielkonfiguration

esphome:
  name: enocean-gateway
  platform: ESP32
  board: esp32dev

wifi:
  ssid: "MeinWLAN"
  password: "MeinPasswort"

mqtt:
  id: mqtt_client
  broker: 192.168.1.10
  port: 1883
  username: "mqtt_user"
  password: "mqtt_pass"

uart:
  id: uart_enocean
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 57600

external_components:
  - source:
      type: local
      path: components

enocean_mqtt:
  id: enocean
  mqtt_id: mqtt_client
  uart_id: uart_enocean
  main_topic: "enocean"
  debug_raw: true
  known_devices:
    - device_id: "AABB1122"
      name: "Fensterkontakt Wohnzimmer"
      eep: "D5-00-01"
    - device_id: "CCDD3344"
      name: "Thermostat Schlafzimmer"
      eep: "A5-20-06"
    - device_id: "EEFF5566"
      name: "Wandschalter Flur"
      eep: "F6-02-01"
    - device_id: "11223344"
      name: "Fensterklinke Bad"
      eep: "F6-10-00"
    - device_id: "55667788"
      name: "CO2 Sensor Büro"
      eep: "A5-09-04"
