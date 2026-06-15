#### Configuration variables
**id** (Optional, [ID](http://https://esphome.io/guides/configuration-types/#id "ID")): Manually specify the ID used for code generation.

**uart_id** (Required, id): The id of the uart to use for reading EnOcean datagrams.

**parse_all_dev** (Optional, Boolean): Activates the parsing of all device datagrams. Default only known devices are parsed.

**known_devices** (Optional, List): List of known devices. All options are required to be set per device entry.

**name** (Required, string): The human readable name of the device.
**device_id** (Required, string): The device identifier or address.
**eep** (Required, string): The EnOcean Equipment Profile to use for payload identification. Check supported EEP for valide options.

### How to use in esphome.yaml

    enocean_mqtt_bridge:
      id: my_enocean_bridge
      uart_id: my_uart
      parse_all_dev: true
      known_devices:
      - device_id: 0xFEF12345
        name: "wohnzimmer_fenster"
        eep: "F6-10-00"
