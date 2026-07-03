#ifndef APP_LIGHT_SERVICE_H
#define APP_LIGHT_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t lightRaw;
    uint16_t threshold;
    uint8_t autoMode;
    uint8_t led0On;
    uint8_t alarmManualOn;
    uint8_t alarmAutoEnabled;
    uint8_t alarmActive;
} AppLightStatus_t;

void AppLightService_Init(void);
void AppLightService_Tick(void);
void AppLightService_GetStatus(AppLightStatus_t *out);

void AppLightService_ToggleMode(void);
void AppLightService_ToggleLed0Manual(void);
void AppLightService_ToggleAlarmManual(void);
void AppLightService_ToggleAlarmAuto(void);
void AppLightService_AdjustThreshold(int16_t delta);
void AppLightService_HandleKey2Pressed(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LIGHT_SERVICE_H */
