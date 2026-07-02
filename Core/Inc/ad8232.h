#ifndef __AD8232_H
#define __AD8232_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

uint16_t AD8232_ReadRaw(void);
float AD8232_ReadVoltage(void);
uint8_t AD8232_IsLeadsOff(void);
uint8_t AD8232_ReadLOP(void);
uint8_t AD8232_ReadLON(void);
uint32_t AD8232_GetLastAdcError(void);

#ifdef __cplusplus
}
#endif

#endif /* __AD8232_H */
