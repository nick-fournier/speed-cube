#ifndef PICO_UPS_H
#define PICO_UPS_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

/** default I2C address **/
#define INA219_ADDRESS (0x43) 

class INA219 {
public:
  INA219(uint8_t addr = INA219_ADDRESS);
  void begin();
  void setCalibration_32V_2A();
  float getBusVoltage_V();
  float getShuntVoltage_mV();
  float getCurrent_mA();
  float getPower_mW();
  float getBatteryPercentage();
  void powerSave(bool on);

private:
  void wireWriteRegister(uint8_t reg, uint16_t value);
  void wireReadRegister(uint8_t reg, uint16_t *value);

  uint8_t ina219_i2caddr;
  uint32_t ina219_calValue;
  uint32_t ina219_currentDivider_mA;
  float ina219_powerMultiplier_mW;
};

#endif // PICO_UPS_H