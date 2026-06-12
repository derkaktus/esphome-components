#include "eep_registry.h"

#include "f6/eep_F6_02_01.h"
#include "f6/eep_F6_10_00.h"

#include "a5/eep_A5_02_05.h"
#include "a5/eep_A5_06_02.h"
#include "a5/eep_A5_07_01.h"
#include "a5/eep_A5_09_04.h"
#include "a5/eep_A5_10_06.h"
#include "a5/eep_A5_20_06.h"

#include "d5/eep_D5_00_01.h"

namespace esphome {
namespace enocean_mqtt {

void register_all_eeps() {
    auto& reg = EepRegistry::instance();

    // ── F6: RPS Telegramme ────────────────────────────────────
    reg.register_eep(std::make_shared<EepF6_02_01>());
    reg.register_eep(std::make_shared<EepF6_10_00>());

    // ── A5: 4BS Telegramme ────────────────────────────────────
    reg.register_eep(std::make_shared<EepA5_02_05>());
    reg.register_eep(std::make_shared<EepA5_06_02>());
    reg.register_eep(std::make_shared<EepA5_07_01>());
    reg.register_eep(std::make_shared<EepA5_09_04>());
    reg.register_eep(std::make_shared<EepA5_10_06>());
    reg.register_eep(std::make_shared<EepA5_20_06>());

    // ── D5: 1BS Telegramme ────────────────────────────────────
    reg.register_eep(std::make_shared<EepD5_00_01>());
}

} // namespace enocean_mqtt
} // namespace esphome
