#include "esphomelib/defines.h"

#ifdef USE_A4988

#include "esphomelib/stepper/a4988.h"
#include "esphomelib/log.h"

ESPHOMELIB_NAMESPACE_BEGIN

namespace stepper {

static const char *TAG = "stepper.a4988";

void A4988::setup() {
  ESP_LOGCONFIG(TAG, "Setting up A4988 stepper driver...");
  if (this->sleep_pin_ != nullptr) {
    this->sleep_pin_->setup();
    this->sleep_pin_->digital_write(false);
  }
  this->step_pin_->setup();
  this->step_pin_->digital_write(false);
  this->dir_pin_->setup();
  this->dir_pin_->digital_write(false);
}
void A4988::loop() {
  if (this->sleep_pin_ != nullptr) {
    this->sleep_pin_->digital_write(!this->has_reached_target());
  }

  int32_t dir = this->should_step_();
  if (dir == 0)
    return;

  this->dir_pin_->digital_write(dir == 1);
  this->step_pin_->digital_write(true);
  delayMicroseconds(5);
  this->step_pin_->digital_write(false);
}
float A4988::get_setup_priority() const {
  return setup_priority::HARDWARE;
}
A4988::A4988(GPIOPin *step_pin, GPIOPin *dir_pin)
    : step_pin_(step_pin), dir_pin_(dir_pin) {

}
void A4988::set_sleep_pin(const GPIOOutputPin &sleep_pin) {
  this->sleep_pin_ = sleep_pin.copy();
}

} // namespace stepper

ESPHOMELIB_NAMESPACE_END

#endif //USE_A4988
