#ifndef APP_ALARM_SERVICE_H
#define APP_ALARM_SERVICE_H

#include "app_sensor_service.h"
#include "bp_service.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    SensorData_t sensor;
    BP_State_t bpState;
    float bpPressure;
    float bpDeflateSpeed;
    float bpSbp;
    float bpDbp;
    uint8_t bpHr;
    uint8_t bpResultValid;
} AppAlarmInput_t;

void AppAlarm_Init(void);
void AppAlarm_Reset(void);
void AppAlarm_Update(const AppAlarmInput_t *input);

#ifdef __cplusplus
}
#endif

#endif /* APP_ALARM_SERVICE_H */
