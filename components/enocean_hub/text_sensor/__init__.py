"""ESPHome  text_sensor  platform: enocean_hub.

Supported EEPs
--------------
  F6-02-01   Rocker Switch, 2 Rocker      -> "action" (last button action as text)
  F6-10-00   Mechanical Handle            -> "position_text" ("closed"/"tilted"/"open")

YAML example
------------
text_sensor:
  - platform: enocean_hub
    enocean_hub_id: my_hub
    sender_id: 0xDEADBEEF
    eep: F6-02-01
    action:
      name: "Rocker last action"

  - platform: enocean_hub
    enocean_hub_id: my_hub
    sender_id: 0xAABBCCDD
    eep: F6-10-00
    position_text:
      name: "Window position text"
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor as ha_text_sensor
from esphome.const import CONF_ID

from .. import CONF_ENOCEAN_HUB_ID, enocean_hub_ns, EnoceanHub

DEPENDENCIES = ["enocean_hub"]

EnoceanDevice = enocean_hub_ns.class_("EnoceanDevice", cg.Component)

CONF_SENDER_ID     = "sender_id"
CONF_EEP           = "eep"
CONF_ACTION        = "action"
CONF_POSITION_TEXT = "position_text"

SUPPORTED_EEPS = ["F6-02-01", "F6-10-00"]

_TS = ha_text_sensor.text_sensor_schema

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EnoceanDevice),
            cv.GenerateID(CONF_ENOCEAN_HUB_ID): cv.use_id(EnoceanHub),
            cv.Required(CONF_SENDER_ID): cv.hex_uint32_t,
            cv.Required(CONF_EEP): cv.one_of(*SUPPORTED_EEPS, upper=True),
            cv.Optional(CONF_ACTION): _TS(),
            cv.Optional(CONF_POSITION_TEXT): _TS(),
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
        lines = [
            "{",
            "  auto _eep = std::make_unique<esphome::enocean_hub::EepF6_02_01>();",
        ]
        if CONF_ACTION in config:
            sens = await ha_text_sensor.new_text_sensor(config[CONF_ACTION])
            lines.append(f"  _eep->set_action_sensor({sens});")
        lines += [f"  {dev}->set_eep(std::move(_eep));", "}"]
        cg.add(cg.RawStatement("\n".join(lines)))

    elif eep == "F6-10-00":
        lines = [
            "{",
            "  auto _eep = std::make_unique<esphome::enocean_hub::EepF6_10_00>();",
        ]
        if CONF_POSITION_TEXT in config:
            sens = await ha_text_sensor.new_text_sensor(config[CONF_POSITION_TEXT])
            lines.append(f"  _eep->set_position_text_sensor({sens});")
        lines += [f"  {dev}->set_eep(std::move(_eep));", "}"]
        cg.add(cg.RawStatement("\n".join(lines)))
