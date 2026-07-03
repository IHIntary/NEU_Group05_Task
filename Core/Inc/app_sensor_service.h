#ifndef APP_SENSOR_SERVICE_H
#define APP_SENSOR_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t ecgRaw;
    uint16_t ecgFiltered;
    uint8_t ecgLeadsOff;
    uint8_t ecgRunning;

    uint16_t pressureRaw;
    uint32_t pressureMv;
    uint32_t pressureMmHgTenths;
    float pressurePercent;
    uint8_t pressureRunning;

    uint32_t ppgIr;
    uint32_t ppgRed;
    uint16_t heartRate;
    float spo2;
    uint16_t pulseValidCount;
    uint8_t pulseProgressPercent;
    uint8_t pulseRunning;
    uint8_t pulseDataValid;
    uint8_t pulseDone;

    uint8_t max30102Ready;

    float chipTempC;
    uint8_t chipTempValid;

    int16_t imuAccMg[3];
    uint8_t imuRunning;
    uint8_t imuReady;
    uint8_t imuDataValid;
    uint8_t imuFallDetected;
    uint8_t imuAlarmActive;
} SensorData_t;

void SensorService_Init(void);
void SensorService_Task(void *argument);
void StartTaskSensorService(void *argument);
void StartTask04(void *argument);

void SensorService_GetData(SensorData_t *out);

void SensorService_SetEcgRunning(uint8_t running);
void SensorService_SetPulseRunning(uint8_t running);
void SensorService_SetPressureRunning(uint8_t running);
void SensorService_SetImuRunning(uint8_t running);
void SensorService_ClearImuAlarm(void);

#ifdef __cplusplus
}
#endif

#endif
