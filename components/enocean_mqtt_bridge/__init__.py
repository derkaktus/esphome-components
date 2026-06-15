import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import uart
from esphome.core import CORE

# Definition des Namespaces und der Klasse
enocean_ns = cg.esphome_ns.namespace('enocean')
EnOceanBridge = enocean_ns.class_('EnOceanBridge', cg.Component, uart.UARTDevice)

# Konfigurationsschlüssel
CONF_KNOWN_DEVICES = "known_devices"
CONF_DEVICE_ID = "device_id"
CONF_NAME = "name"
CONF_EEP = "eep"
CONF_PARSE_ALL_DEV = "parse_all_dev"
CONF_MAX_DEV = "max_dev"  # NEU: Option für maximale Geräteanzahl im Flash

# Schema für ein einzelnes bekanntes Gerät
KNOWN_DEVICE_SCHEMA = cv.Schema({
    cv.Required(CONF_DEVICE_ID): cv.hex_uint32_t,
    cv.Required(CONF_NAME): cv.string,
    cv.Required(CONF_EEP): cv.string,
})

# Hilfsfunktion zur dynamischen Ermittlung des Standardwerts für max_dev
def validate_config(config):
    if CONF_MAX_DEV not in config:
        # NEU: Wenn ESP32, setze Default auf 40, ansonsten (ESP8266) auf 20
        config[CONF_MAX_DEV] = 40 if CORE.is_esp32 else 20
    return config

# Gesamtes Konfigurationsschema der Brücke
CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(EnOceanBridge),
        cv.Optional(CONF_PARSE_ALL_DEV, default=False): cv.boolean,
        cv.Optional(CONF_MAX_DEV): cv.int_range(min=1, max=100),  # Nur zur Sicherehit eine Range
        cv.Optional(CONF_KNOWN_DEVICES): cv.ensure_list(KNOWN_DEVICE_SCHEMA),
    }).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA),
    validate_config
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # Übergabe der parse_all_dev Einstellung
    cg.add(var.set_parse_all_dev(config[CONF_PARSE_ALL_DEV]))
    
    # NEU: Übergabe der maximalen Geräteanzahl an die C++ Klasse
    cg.add(var.set_max_dev(config[CONF_MAX_DEV]))

    # Übergabe der in der YAML definierten statischen Geräte
    if CONF_KNOWN_DEVICES in config:
        for device in config[CONF_KNOWN_DEVICES]:
            # Diese Geräte werden temporär an C++ übergeben, damit sie dort beim
            # Booten in den NVS (Flash) synchronisiert werden können.
            cg.add(var.add_yaml_device(device[CONF_DEVICE_ID], device[CONF_NAME], device[CONF_EEP]))
