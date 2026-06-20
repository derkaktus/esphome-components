"""ESPHome  sensor  platform: enocean_hub.

Supported EEPs
--------------
  F6-10-00   Mechanical Handle (window/door handle)

YAML example
------------
sensor:
  - platform: enocean_hub
    enocean_hub_id: my_hub
    sender_id: 0xAABBCCDD
    eep: F6-10-00
    position:
      name: "Window position"
      # 0 = closed, 1 = tilted, 2 = open
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor as ha_sensor
from esphome.const import CONF_ID

from .. import CONF_ENOCEAN_HUB_ID, enocean_hub_ns, EnoceanHub

DEPENDENCIES = ["enocean_hub"]

EnoceanDevice = enocean_hub_ns.class_("EnoceanDevice", cg.Component)

CONF_SENDER_ID = "sender_id"
CONF_EEP       = "eep"
CONF_POSITION  = "position"

SUPPORTED_EEPS = ["F6-10-00"]

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EnoceanDevice),
            cv.GenerateID(CONF_ENOCEAN_HUB_ID): cv.use_id(EnoceanHub),
            cv.Required(CONF_SENDER_ID): cv.hex_uint32_t,
            cv.Required(CONF_EEP): cv.one_of(*SUPPORTED_EEPS, upper=True),
            cv.Optional(CONF_POSITION): ha_sensor.sensor_schema(
                icon="mdi:window-open-variant",
                accuracy_decimals=0,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_ENOCEAN_HUB_ID])
    dev = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(dev, config)

    cg.add(dev.set_hub(hub))
    cg.add(dev.set_sender_id(config[CONF_SENDER_ID]))

    eep = config[CONF_EEP]

    if eep == "F6-10-00":
        lines = [
            "{",
            "  auto _eep = std::make_unique<esphome::enocean_hub::EepF6_10_00>();",
        ]

        if CONF_POSITION in config:
            sens = await ha_sensor.new_sensor(config[CONF_POSITION])
            lines.append(f"  _eep->set_position_sensor({sens});")

        lines += [
            f"  {dev}->set_eep(std::move(_eep));",
            "}",
        ]
        cg.add(cg.RawStatement("\n".join(lines)))
