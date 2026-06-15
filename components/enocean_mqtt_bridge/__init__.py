import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_NAME

DEPENDENCIES = ['uart', 'mqtt']

# C++ Namespace und Klasse referenzieren
enocean_ns = cg.esphome_ns.namespace('enocean_mqtt_bridge')
EnOceanBridge = enocean_ns.class_('EnOceanBridge', cg.Component, uart.UARTDevice)

CONF_KNOWN_DEVICES = 'known_devices'
CONF_DEVICE_ID = 'device_id'
CONF_EEP = 'eep'
CONF_PARSE_ALL_DEV = 'parse_all_dev'

# Schema für ein einzelnes Gerät aus der known_devices Liste
DEVICE_SCHEMA = cv.Schema({
    cv.Required(CONF_DEVICE_ID): cv.hex_int,
    cv.Required(CONF_NAME): cv.string_strict,
    cv.Required(CONF_EEP): cv.string_strict,
})

# Haupt-Schema für die Komponente
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EnOceanBridge),
    cv.Optional(CONF_KNOWN_DEVICES, default=[]): cv.ensure_list(DEVICE_SCHEMA),
    cv.Optional(CONF_PARSE_ALL_DEV, default=False): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    # C++ Objekt instanziieren
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_parse_all_dev(config[CONF_PARSE_ALL_DEV]))
    
    # Bekannte Geräte aus der YAML dem C++ Objekt hinzufügen
    for device in config[CONF_KNOWN_DEVICES]:
        cg.add(var.add_known_device(
            device[CONF_DEVICE_ID], 
            device[CONF_NAME], 
            device[CONF_EEP]
        ))
