#ifndef ESPHOME_BINARY_SENSOR_TTP229_H
#define ESPHOME_BINARY_SENSOR_TTP229_H

#include "esphome/defines.h"

#ifdef USE_TTP229

#include "esphome/binary_sensor/binary_sensor.h"
#include "esphome/i2c_component.h"
#include "esphome/component.h"

ESPHOME_NAMESPACE_BEGIN

namespace binary_sensor {

class TTP229Channel : public binary_sensor::BinarySensor {
 public:
  TTP229Channel(const std::string &name, int channel_num = 0);
  void process(const uint16_t *data, const uint16_t *last_data);

 protected:
  int channel_ = 0;
};

class TTP229Component : public Component, public I2CDevice {
 public:
  TTP229Component(I2CComponent *parent, uint8_t scl_pin, uint8_t sdo_pin, uint8_t address = 0x5A);
  binary_sensor::TTP229Channel *add_channel(binary_sensor::TTP229Channel *channel);
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void loop() override;

 protected:
  std::vector<TTP229Channel *> channels_{};
  uint8_t scl_pin_;
  uint8_t sdo_pin_;
  uint16_t lasttouched_ = 0;
  uint16_t currtouched_ = 0;
  enum ErrorCode {
    NONE = 0,
    COMMUNICATION_FAILED,
    WRONG_CHIP_STATE,
  } error_code_{NONE};
  void process_(uint16_t *data, uint16_t *last_data);
  uint16_t read_channels_();
};

}  // namespace binary_sensor

ESPHOME_NAMESPACE_END

#endif  // USE_TTP229

#endif  // ESPHOME_BINARY_SENSOR_TTP229_H
