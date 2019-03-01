// Based on:
//  - https://cdn-shop.adafruit.com/datasheets/BST-BMP280_DS001-10.pdf
//  - https://github.com/adafruit/Adafruit_BMP280_Library
//  - https://github.com/sparkfun/SparkFun_BMP280_Arduino_Library

#include "esphome/defines.h"

#ifdef USE_BMP280

#include "esphome/sensor/bmp280_component.h"
#include "esphome/espmath.h"
#include "esphome/log.h"

ESPHOME_NAMESPACE_BEGIN

namespace sensor {

static const char *TAG = "sensor.bmp280";

static const uint8_t BMP280_REGISTER_STATUS = 0xF3;
static const uint8_t BMP280_REGISTER_CONTROL = 0xF4;
static const uint8_t BMP280_REGISTER_CONFIG = 0xF5;
static const uint8_t BMP280_REGISTER_PRESSUREDATA = 0xF7;
static const uint8_t BMP280_REGISTER_TEMPDATA = 0xFA;

static const uint8_t BMP280_MODE_FORCED = 0b01;

inline uint16_t combine_bytes(uint8_t msb, uint8_t lsb) { return ((msb & 0xFF) << 8) | (lsb & 0xFF); }

BMP280Component::BMP280Component(I2CComponent *parent, const std::string &temperature_name,
                                 const std::string &pressure_name, uint8_t address, uint32_t update_interval)
    : PollingComponent(update_interval),
      I2CDevice(parent, address),
      temperature_sensor_(new BMP280TemperatureSensor(temperature_name, this)),
      pressure_sensor_(new BMP280PressureSensor(pressure_name, this)) {}

static const char *oversampling_to_str(BMP280Oversampling oversampling) {
  switch (oversampling) {
    case BMP280_OVERSAMPLING_NONE:
      return "None";
    case BMP280_OVERSAMPLING_1X:
      return "1x";
    case BMP280_OVERSAMPLING_2X:
      return "2x";
    case BMP280_OVERSAMPLING_4X:
      return "4x";
    case BMP280_OVERSAMPLING_8X:
      return "8x";
    case BMP280_OVERSAMPLING_16X:
      return "16x";
    default:
      return "UNKNOWN";
  }
}

static const char *iir_filter_to_str(BMP280IIRFilter filter) {
  switch (filter) {
    case BMP280_IIR_FILTER_OFF:
      return "OFF";
    case BMP280_IIR_FILTER_2X:
      return "2x";
    case BMP280_IIR_FILTER_4X:
      return "4x";
    case BMP280_IIR_FILTER_8X:
      return "8x";
    case BMP280_IIR_FILTER_16X:
      return "16x";
    default:
      return "UNKNOWN";
  }
}

void BMP280Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BMP280...");
  uint8_t chip_id = 0;
  if (!this->read_byte(0xD0, &chip_id)) {
    this->error_code_ = COMMUNICATION_FAILED;
    this->mark_failed();
    return;
  }
  if (chip_id != 0x58) {
    this->error_code_ = WRONG_CHIP_ID;
    this->mark_failed();
    return;
  }

  // Read calibration
  this->calibration_.t1 = read_u16_le(0x88);
  this->calibration_.t2 = read_s16_le(0x8A);
  this->calibration_.t3 = read_s16_le(0x8C);

  this->calibration_.p1 = read_u16_le(0x8E);
  this->calibration_.p2 = read_s16_le(0x90);
  this->calibration_.p3 = read_s16_le(0x92);
  this->calibration_.p4 = read_s16_le(0x94);
  this->calibration_.p5 = read_s16_le(0x96);
  this->calibration_.p6 = read_s16_le(0x98);
  this->calibration_.p7 = read_s16_le(0x9A);
  this->calibration_.p8 = read_s16_le(0x9C);
  this->calibration_.p9 = read_s16_le(0x9E);

  uint8_t config_register = 0;
  if (!this->read_byte(BMP280_REGISTER_CONFIG, &config_register)) {
    this->mark_failed();
    return;
  }
  config_register &= ~0b11111100;
  config_register |= 0b000 << 5;  // 0.5 ms standby time
  config_register |= (this->iir_filter_ & 0b111) << 2;
  if (!this->write_byte(BMP280_REGISTER_CONFIG, config_register)) {
    this->mark_failed();
    return;
  }
}
void BMP280Component::dump_config() {
  ESP_LOGCONFIG(TAG, "BMP280:");
  LOG_I2C_DEVICE(this);
  switch (this->error_code_) {
    case COMMUNICATION_FAILED:
      ESP_LOGE(TAG, "Communication with BMP280 failed!");
      break;
    case WRONG_CHIP_ID:
      ESP_LOGE(TAG, "BMP280 has wrong chip ID! Is it a BME280?");
      break;
    case NONE:
    default:
      break;
  }
  ESP_LOGCONFIG(TAG, "  IIR Filter: %s", iir_filter_to_str(this->iir_filter_));
  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  ESP_LOGCONFIG(TAG, "    Oversampling: %s", oversampling_to_str(this->temperature_oversampling_));
  LOG_SENSOR("  ", "Pressure", this->pressure_sensor_);
  ESP_LOGCONFIG(TAG, "    Oversampling: %s", oversampling_to_str(this->pressure_oversampling_));
}
float BMP280Component::get_setup_priority() const { return setup_priority::HARDWARE_LATE; }

inline uint8_t oversampling_to_time(BMP280Oversampling over_sampling) { return (1 << uint8_t(over_sampling)) >> 1; }

void BMP280Component::update() {
  // Enable sensor
  ESP_LOGV(TAG, "Sending conversion request...");
  uint8_t meas_register = 0;
  meas_register |= (this->temperature_oversampling_ & 0b111) << 5;
  meas_register |= (this->pressure_oversampling_ & 0b111) << 2;
  meas_register |= 0b01;  // Forced mode
  if (!this->write_byte(BMP280_REGISTER_CONTROL, meas_register)) {
    this->status_set_warning();
    return;
  }

  float meas_time = 1;
  meas_time += 2.3f * oversampling_to_time(this->temperature_oversampling_);
  meas_time += 2.3f * oversampling_to_time(this->pressure_oversampling_) + 0.575f;

  this->set_timeout("data", uint32_t(ceilf(meas_time)), [this]() {
    int32_t t_fine = 0;
    float temperature = this->read_temperature_(&t_fine);
    if (isnan(temperature)) {
      ESP_LOGW(TAG, "Invalid temperature, cannot read pressure values.");
      this->status_set_warning();
      return;
    }
    float pressure = this->read_pressure_(t_fine);

    ESP_LOGD(TAG, "Got temperature=%.1f°C pressure=%.1fhPa", temperature, pressure);
    this->temperature_sensor_->publish_state(temperature);
    this->pressure_sensor_->publish_state(pressure);
    this->status_clear_warning();
  });
}

float BMP280Component::read_temperature_(int32_t *t_fine) {
  uint8_t data[3];
  if (!this->read_bytes(BMP280_REGISTER_TEMPDATA, data, 3))
    return NAN;
  int32_t adc = ((data[0] & 0xFF) << 16) | ((data[1] & 0xFF) << 8) | (data[2] & 0xFF);
  adc >>= 4;
  if (adc == 0x80000)
    // temperature was disabled
    return NAN;

  const int32_t t1 = this->calibration_.t1;
  const int32_t t2 = this->calibration_.t2;
  const int32_t t3 = this->calibration_.t3;

  int32_t var1 = (((adc >> 3) - (t1 << 1)) * t2) >> 11;
  int32_t var2 = (((((adc >> 4) - t1) * ((adc >> 4) - t1)) >> 12) * t3) >> 14;
  *t_fine = var1 + var2;

  float temperature = (*t_fine * 5 + 128) >> 8;
  return temperature / 100.0f;
}

float BMP280Component::read_pressure_(int32_t t_fine) {
  uint8_t data[3];
  if (!this->read_bytes(BMP280_REGISTER_PRESSUREDATA, data, 3))
    return NAN;
  int32_t adc = ((data[0] & 0xFF) << 16) | ((data[1] & 0xFF) << 8) | (data[2] & 0xFF);
  adc >>= 4;
  if (adc == 0x80000)
    // pressure was disabled
    return NAN;
  const int64_t p1 = this->calibration_.p1;
  const int64_t p2 = this->calibration_.p2;
  const int64_t p3 = this->calibration_.p3;
  const int64_t p4 = this->calibration_.p4;
  const int64_t p5 = this->calibration_.p5;
  const int64_t p6 = this->calibration_.p6;
  const int64_t p7 = this->calibration_.p7;
  const int64_t p8 = this->calibration_.p8;
  const int64_t p9 = this->calibration_.p9;

  int64_t var1, var2, p;
  var1 = int64_t(t_fine) - 128000;
  var2 = var1 * var1 * p6;
  var2 = var2 + ((var1 * p5) << 17);
  var2 = var2 + (p4 << 35);
  var1 = ((var1 * var1 * p3) >> 8) + ((var1 * p2) << 12);
  var1 = ((int64_t(1) << 47) + var1) * p1 >> 33;

  if (var1 == 0)
    return NAN;

  p = 1048576 - adc;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (p9 * (p >> 13) * (p >> 13)) >> 25;
  var2 = (p8 * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (p7 << 4);
  return (p / 256.0f) / 100.0f;
}
BMP280TemperatureSensor *BMP280Component::get_temperature_sensor() const { return this->temperature_sensor_; }
BMP280PressureSensor *BMP280Component::get_pressure_sensor() const { return this->pressure_sensor_; }
void BMP280Component::set_temperature_oversampling(BMP280Oversampling temperature_over_sampling) {
  this->temperature_oversampling_ = temperature_over_sampling;
}
void BMP280Component::set_pressure_oversampling(BMP280Oversampling pressure_over_sampling) {
  this->pressure_oversampling_ = pressure_over_sampling;
}
void BMP280Component::set_iir_filter(BMP280IIRFilter iir_filter) { this->iir_filter_ = iir_filter; }
uint8_t BMP280Component::read_u8(uint8_t register_) {
  uint8_t data = 0;
  this->read_byte(register_, &data);
  return data;
}
uint16_t BMP280Component::read_u16_le(uint8_t register_) {
  uint16_t data = 0;
  this->read_byte_16(register_, &data);
  return (data >> 8) | (data << 8);
}
int16_t BMP280Component::read_s16_le(uint8_t register_) { return this->read_u16_le(register_); }

}  // namespace sensor

ESPHOME_NAMESPACE_END

#endif  // USE_BMP280
