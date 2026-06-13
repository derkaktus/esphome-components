#pragma once

#include "esphome/core/component.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/components/uart/uart.h"

#include "enocean_uart.h"
#include "enocean_known_devices.h"
#include "eep_registry.h"

#include <string>
#include <vector>

namespace esphome {
namespace enocean_mqtt {

class EnoceanMqttComponent
    : public Component
    , public uart::UARTDevice {
public:

    void setup()   override;
    void loop()    override;
    float get_setup_priority() const override {
        return setup_priority::AFTER_WIFI;
    }

    void set_mqtt(mqtt::MQTTClientComponent* mqtt)  { this->mqtt_     = mqtt;  }
    void set_uart(uart::UARTComponent* uart)        { this->set_uart_parent(uart); }
    void set_main_topic(const std::string& topic)  { this->main_topic_ = topic; }
    void set_debug_raw(bool debug)                 { this->debug_raw_  = debug; }

    void add_known_device(
        const std::string& id,
        const std::string& name,
        const std::string& eep
    );

private:

    mqtt::MQTTClientComponent* mqtt_     {nullptr};

    std::string main_topic_ {"enocean"};
    bool        debug_raw_  {true};

    KnownDeviceStore known_devices_;

    EnoceanUartParser uart_parser_;

    void handle_telegram(const EnoceanTelegram& telegram);

    void dispatch_eep(
        const std::string& eep,
        const std::string& device_id,
        const uint8_t*     data,
        uint8_t            data_len
    );

    void publish_device_meta(
        const std::string&    device_id,
        const KnownDevice*    device
    );

    void publish_debug_raw(const EnoceanTelegram& telegram);

    void handle_mqtt_command(
        const std::string& topic,
        const std::string& payload
    );

    std::string device_topic(const std::string& device_id) const {
        return main_topic_ + "/" + device_id;
    }

    void mqtt_publish(
        const std::string& topic,
        const std::string& payload,
        bool retain = false
    );
};

} // namespace enocean_mqtt
} // namespace esphome