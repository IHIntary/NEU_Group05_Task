#ifndef APP_I2C_BUS_H
#define APP_I2C_BUS_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AppI2cBus_Init(void);
HAL_StatusTypeDef AppI2c2_MasterTransmit(uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef AppI2c2_MasterReceive(uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef AppI2c2_MemRead8(uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif
