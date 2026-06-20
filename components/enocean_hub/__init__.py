"""ESPHome EnOcean Hub component."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.components import text_sensor as ha_text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
MULTI_CONF = False

CONF_ENOCEAN_HUB_ID = "enocean_hub_id"
CONF_DEBUG_DEV = "debug_dev"
CONF_DEBUG_SENSOR = "debug_sensor"

enocean_hub_ns = cg.esphome_ns.namespace("enocean_hub")
EnoceanHub = enocean_hub_ns.class_("EnoceanHub", cg.Component, uart.UARTDevice)

def _validate_debug(config):
    if CONF_DEBUG_SENSOR in config and not config.get(CONF_DEBUG_DEV):
        raise cv.Invalid(
            f"'{CONF_DEBUG_SENSOR}' requires '{CONF_DEBUG_DEV}: true'"
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EnoceanHub),
            cv.Optional(CONF_DEBUG_DEV, default=False): cv.boolean,
            cv.Optional(CONF_DEBUG_SENSOR): ha_text_sensor.text_sensor_schema(),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    _validate_debug,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if config.get(CONF_DEBUG_DEV) and CONF_DEBUG_SENSOR in config:
        sens = await ha_text_sensor.new_text_sensor(config[CONF_DEBUG_SENSOR])
        cg.add(var.set_debug_sensor(sens))
