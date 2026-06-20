#pragma once

#include "enocean_hub.h"

namespace esphome {
namespace enocean_hub {

/// Abstract base for all EEP parsers.
/// Subclasses override parse() and attach their own sensors.
class EepBase {
 public:
  virtual ~EepBase() = default;

  /// Called by the hub for every telegram belonging to this device.
  virtual void parse(const EnoceanTelegram &tg) = 0;

  /// Human-readable EEP string, e.g. "F6-02-01"
  virtual const char *eep_string() const = 0;
};

}  // namespace enocean_hub
}  // namespace esphome
