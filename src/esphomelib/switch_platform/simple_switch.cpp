//
// Created by Otto Winter on 20.01.18.
//

#include "esphomelib/switch_platform/simple_switch.h"

#include "esphomelib/log.h"

namespace esphomelib {

namespace switch_platform {

SimpleSwitch::SimpleSwitch(output::BinaryOutput *output)
    : output_(output) { }

void SimpleSwitch::write_state(bool state) {
  assert(this->output_ != nullptr);
  if (state) this->output_->enable();
  else this->output_->disable();
  this->publish_state(state);
}

} // namespace switch_platform

} // namespace esphomelib
