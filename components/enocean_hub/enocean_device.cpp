#include "enocean_device.h"
#include "esphome/core/log.h"

namespace esphome {
namespace enocean_hub {

static const char *const TAG = "enocean_device";

void EnoceanDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "EnOcean Device:");
  ESP_LOGCONFIG(TAG, "  Sender ID : 0x%08X", sender_id_);
  if (eep_)
    ESP_LOGCONFIG(TAG, "  EEP       : %s", eep_->eep_string());
}

}  // namespace enocean_hub
}  // namespace esphome
