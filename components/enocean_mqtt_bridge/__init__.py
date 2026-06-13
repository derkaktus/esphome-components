import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_NAME

DEPENDENCIES = ['uart', 'mqtt']

enocean_ns = cg.esphome_ns.namespace('enocean_mqtt_bridge')
EnOceanBridge = enocean_ns.class_('EnOceanBridge', cg.Component, uart.UARTDevice)

CONF_KNOWN_DEVICES = 'known_devices'
CONF_DEVICE_ID = 'device_id'
CONF_EEP = 'eep'

# Schema für ein einzelnes Gerät
DEVICE_SCHEMA = cv.Schema({
    cv.Required(CONF_DEVICE_ID): cv.hex_uint32_t,
    cv.Optional(CONF_NAME, default=""): cv.string,
    cv.Optional(CONF_EEP, default=""): cv.string,
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EnOceanBridge),
    cv.Optional(CONF_KNOWN_DEVICES, default=[]): cv.ensure_list(DEVICE_SCHEMA),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # Geräte an C++ übergeben
    for device in config[CONF_KNOWN_DEVICES]:
        cg.add(var.add_known_device(device[CONF_DEVICE_ID], device[CONF_NAME], device[CONF_EEP]))
