import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import mqtt, uart
from esphome.const import CONF_ID

DEPENDENCIES = ["mqtt", "uart"]
AUTO_LOAD  = ["mqtt", "uart"]

CONF_MQTT_ID      = "mqtt_id"
CONF_UART_ID      = "uart_id"
CONF_MAIN_TOPIC   = "main_topic"
CONF_DEBUG_RAW    = "debug_raw"
CONF_KNOWN_DEVICES = "known_devices"

CONF_DEVICE_ID    = "device_id"
CONF_DEVICE_NAME  = "name"
CONF_DEVICE_EEP   = "eep"

enocean_mqtt_ns   = cg.esphome_ns.namespace("enocean_mqtt")
EnoceanMqttComponent = enocean_mqtt_ns.class_(
    "EnoceanMqttComponent",
    cg.Component
)

KNOWN_DEVICE_SCHEMA = cv.Schema({
    cv.Required(CONF_DEVICE_ID):   cv.string,
    cv.Required(CONF_DEVICE_NAME): cv.string,
    cv.Required(CONF_DEVICE_EEP):  cv.string,
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID():                        cv.declare_id(EnoceanMqttComponent),
    cv.Required(CONF_MQTT_ID):              cv.use_id(mqtt.MQTTClientComponent),
    cv.Required(CONF_UART_ID):              cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_MAIN_TOPIC,
                default="enocean"):         cv.string,
    cv.Optional(CONF_DEBUG_RAW,
                default=True):              cv.boolean,
    cv.Optional(CONF_KNOWN_DEVICES,
                default=[]):                cv.ensure_list(KNOWN_DEVICE_SCHEMA),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # MQTT
    mqtt_client = await cg.get_variable(config[CONF_MQTT_ID])
    cg.add(var.set_mqtt(mqtt_client))

    # UART
    uart_bus = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart(uart_bus))

    # Optionen
    cg.add(var.set_main_topic(config[CONF_MAIN_TOPIC]))
    cg.add(var.set_debug_raw(config[CONF_DEBUG_RAW]))

    # Known Devices
    for device in config[CONF_KNOWN_DEVICES]:
        cg.add(var.add_known_device(
            device[CONF_DEVICE_ID],
            device[CONF_DEVICE_NAME],
            device[CONF_DEVICE_EEP]
        ))
