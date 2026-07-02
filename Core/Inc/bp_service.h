#ifndef BP_SERVICE_H
#define BP_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BP_STATE_IDLE = 0,
    BP_STATE_WAIT_INFLATE,
    BP_STATE_INFLATING,
    BP_STATE_READY_DEFLATE,
    BP_STATE_DEFLATING,
    BP_STATE_CALC,
    BP_STATE_DONE,
    BP_STATE_ERR
} BP_State_t;

typedef struct
{
    float press_mmhg;
    float amp;
    uint32_t tick_ms;
} BP_WavePoint_t;

void BP_Service_Init(void);
void BP_Service_Start(void);
void BP_Service_Reset(void);
void BP_Service_Tick(void);
void BP_Service_OnAdcSamples(const uint16_t *buf, uint16_t len);
void BP_Service_SetCalibration(uint16_t adc1, float mmHg1, uint16_t adc2, float mmHg2);
void BP_Service_ResetCalibration(void);
float BP_Service_CalcPressureFromAdc(uint16_t adc);

float BP_Service_GetPressure(void);
float BP_Service_GetDeflateSpeed(void);
float BP_Service_GetSBP(void);
float BP_Service_GetDBP(void);
float BP_Service_GetMAP(void);
uint8_t BP_Service_GetHR(void);
uint8_t BP_Service_IsResultValid(void);
BP_State_t BP_Service_GetState(void);
const char* BP_Service_GetHintText(void);
const char* BP_Service_GetStateText(void);

#ifdef __cplusplus
}
#endif

#endif /* BP_SERVICE_H */
