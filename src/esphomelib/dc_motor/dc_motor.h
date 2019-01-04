#ifndef ESPHOMELIB_DC_MOTOR_DC_MOTOR_H
#define ESPHOMELIB_DC_MOTOR_DC_MOTOR_H

#include "esphomelib/defines.h"

#ifdef USE_DC_MOTOR

#include "esphomelib/component.h"
#include "esphomelib/automation.h"

ESPHOMELIB_NAMESPACE_BEGIN

namespace dc_motor {

#define LOG_STEPPER(this) \
    ESP_LOGCONFIG(TAG, "Sensor type: ", this->sensor_type); 

class Dc_motor {
 public:
  void move();
  void stop();
  void set_direction(bool clockwise);
  
 protected:
  int sensor_type = 0;  //no sensor
};


} // namespace dc_motor

ESPHOMELIB_NAMESPACE_END

#endif //USE_DC_MOTOR

#endif //ESPHOMELIB_DC_MOTOR_DC_MOTOR_H
