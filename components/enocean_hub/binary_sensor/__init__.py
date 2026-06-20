"""ESPHome  binary_sensor  platform: enocean_hub.

Supported EEPs
--------------
  F6-02-01   Rocker Switch, 2 Rocker

YAML example
------------
binary_sensor:
  - platform: enocean_hub
    enocean_hub_id: my_hub
    sender_id: 0xDEADBEEF
    eep: F6-02-01
    button_a_top:
      name: "Rocker A – top"
    button_a_bottom:
      name: "Rocker A – bottom"
    button_b_top:
      name: "Rocker B – top"
    button_b_bottom:
      name: "Rocker B – bottom"
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor as ha_binary_sensor
from esphome.const import CONF_ID

from .. import CONF_ENOCEAN_HUB_ID, enocean_hub_ns, EnoceanHub

DEPENDENCIES = ["enocean_hub"]

EnoceanDevice = enocean_hub_ns.class_("EnoceanDevice", cg.Component)

CONF_SENDER_ID       = "sender_id"
CONF_EEP             = "eep"
CONF_BUTTON_A_TOP    = "button_a_top"
CONF_BUTTON_A_BOTTOM = "button_a_bottom"
CONF_BUTTON_B_TOP    = "button_b_top"
CONF_BUTTON_B_BOTTOM = "button_b_bottom"

_BS = ha_binary_sensor.binary_sensor_schema

SUPPORTED_EEPS = ["F6-02-01"]

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EnoceanDevice),
            cv.GenerateID(CONF_ENOCEAN_HUB_ID): cv.use_id(EnoceanHub),
            cv.Required(CONF_SENDER_ID): cv.hex_uint32_t,
            cv.Required(CONF_EEP): cv.one_of(*SUPPORTED_EEPS, upper=True),
            # F6-02-01 buttons (all optional – include only what you need)
            cv.Optional(CONF_BUTTON_A_TOP):    _BS(),
            cv.Optional(CONF_BUTTON_A_BOTTOM): _BS(),
            cv.Optional(CONF_BUTTON_B_TOP):    _BS(),
            cv.Optional(CONF_BUTTON_B_BOTTOM): _BS(),
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

    if eep == "F6-02-01":
        # Emit inline C++ to create the EEP object, wire sensors, call set_eep()
        lines = [
            "{",
            "  auto _eep = std::make_unique<esphome::enocean_hub::EepF6_02_01>();",
        ]

        btn_map = {
            CONF_BUTTON_A_TOP:    "set_button_a_top",
            CONF_BUTTON_A_BOTTOM: "set_button_a_bottom",
            CONF_BUTTON_B_TOP:    "set_button_b_top",
            CONF_BUTTON_B_BOTTOM: "set_button_b_bottom",
        }
        for conf_key, method in btn_map.items():
            if conf_key in config:
                sens = await ha_binary_sensor.new_binary_sensor(config[conf_key])
                # sens is a C++ pointer expression; embed its id string
                lines.append(f"  _eep->{method}({sens});")

        lines += [
            f"  {dev}->set_eep(std::move(_eep));",
            "}",
        ]
        cg.add(cg.RawStatement("\n".join(lines)))
