#ifndef APP_SERIAL_TIME_SERVICE_H
#define APP_SERIAL_TIME_SERVICE_H

#include "app_rtc_service.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AppSerialTimeService_Init(void);
void AppSerialTimeService_OnRxByte(uint8_t byte);
uint8_t AppSerialTimeService_GetPendingDateTime(AppRtcDateTime_t *out);

#ifdef __cplusplus
}
#endif

#endif /* APP_SERIAL_TIME_SERVICE_H */
