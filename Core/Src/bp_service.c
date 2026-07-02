#include "bp_service.h"
#include "app_sensor_service.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define BP_TARGET_PRESS_LOW       180.0f
#define BP_TARGET_PRESS_HIGH      190.0f
#define BP_STOP_PRESS             40.0f
#define BP_MIN_VALID_POINTS       8U
#define BP_SBP_COEFF              0.5f
#define BP_DBP_COEFF              0.8f
#define BP_DEFLATE_SPEED_MIN      2.0f
#define BP_DEFLATE_SPEED_MAX      3.5f

#define BP_ADC_MAX_VALUE          4095.0f
#define BP_DEFAULT_CAL_ADC1       70U
#define BP_DEFAULT_CAL_MMHG1      0.0f
#define BP_DEFAULT_CAL_ADC2       410U
#define BP_DEFAULT_CAL_MMHG2      100.0f

#define BP_USE_USER_CALIBRATION   1U
#define BP_USER_CAL_ADC1          70U
#define BP_USER_CAL_MMHG1         0.0f
#define BP_USER_CAL_ADC2          365U
#define BP_USER_CAL_MMHG2         120.0f
#define BP_WAVE_POINT_MAX         96U
#define BP_PEAK_REFRACTORY_MS     350U
#define BP_SPEED_INTERVAL_MS      500U
#define BP_MIN_OSC_AMP            0.50f

#define BP_VALID_SBP_MIN          40.0f
#define BP_VALID_SBP_MAX          220.0f
#define BP_VALID_DBP_MIN          20.0f
#define BP_VALID_DBP_MAX          140.0f
#define BP_VALID_MAP_MIN          25.0f
#define BP_VALID_MAP_MAX          180.0f
#define BP_VALID_HR_MIN           40U
#define BP_VALID_HR_MAX           180U
#define BP_DEBUG_ENABLE           0U

typedef struct
{
    BP_State_t state;
    float pressure;
    float filteredPressure;
    float deflateSpeed;
    float map;
    float sbp;
    float dbp;
    uint8_t hr;
    uint8_t resultValid;
    uint16_t lastAdc;
    uint8_t hasAdc;
    uint16_t calAdc1;
    uint16_t calAdc2;
    float calMmHg1;
    float calMmHg2;

    float lastSpeedPressure;
    uint32_t lastSpeedTick;
    float oscDc;
    float prevAbsOsc;
    uint32_t lastPeakTick;
    BP_WavePoint_t wave[BP_WAVE_POINT_MAX];
    uint16_t waveCount;
} BP_Context_t;

static BP_Context_t bp;

static const char *BP_StateToText(BP_State_t state);

#if (BP_DEBUG_ENABLE != 0U)
#define BP_DEBUG_PRINTF(...)      printf(__VA_ARGS__)
#else
#define BP_DEBUG_PRINTF(...)
#endif

static void BP_LoadDefaultCalibration(void)
{
#if (BP_USE_USER_CALIBRATION != 0U)
    bp.calAdc1 = BP_USER_CAL_ADC1;
    bp.calMmHg1 = BP_USER_CAL_MMHG1;
    bp.calAdc2 = BP_USER_CAL_ADC2;
    bp.calMmHg2 = BP_USER_CAL_MMHG2;
#else
    bp.calAdc1 = BP_DEFAULT_CAL_ADC1;
    bp.calMmHg1 = BP_DEFAULT_CAL_MMHG1;
    bp.calAdc2 = BP_DEFAULT_CAL_ADC2;
    bp.calMmHg2 = BP_DEFAULT_CAL_MMHG2;
#endif
}

static void BP_StopPressureSampling(void)
{
    SensorService_SetPressureRunning(0U);
}

static void BP_SetState(BP_State_t state, const char *reason)
{
    if (bp.state != state)
    {
        BP_DEBUG_PRINTF("[BP] state %s -> %s pressure=%.1f speed=%.1f wave=%u",
                        BP_StateToText(bp.state),
                        BP_StateToText(state),
                        bp.pressure,
                        bp.deflateSpeed,
                        bp.waveCount);
        if (reason != 0)
        {
            BP_DEBUG_PRINTF(" reason=%s", reason);
        }
        BP_DEBUG_PRINTF("\r\n");
    }

    bp.state = state;
}

float BP_Service_CalcPressureFromAdc(uint16_t adc)
{
    float adcSpan = (float)bp.calAdc2 - (float)bp.calAdc1;
    float pressureSpan = bp.calMmHg2 - bp.calMmHg1;
    float pressure;

    /*
     * TODO: Calibrate for the actual MSP20 analog front-end.
     * Two-point formula:
     *   pressure = mmHg1 + (adc - adc1) * (mmHg2 - mmHg1) / (adc2 - adc1)
     * Default points keep the old transparent placeholder:
     *   raw 0 -> 0 mmHg, raw 4095 -> 300 mmHg.
     * Replace defaults or call BP_Service_SetCalibration() with measured points,
     * for example raw at 0 mmHg and raw at 80 mmHg.
     */
    if ((adcSpan > -1.0f) && (adcSpan < 1.0f))
    {
        adcSpan = BP_ADC_MAX_VALUE;
        pressureSpan = BP_DEFAULT_CAL_MMHG2;
    }

    pressure = bp.calMmHg1 + (((float)adc - (float)bp.calAdc1) * pressureSpan) / adcSpan;

    if (pressure < 0.0f)
    {
        pressure = 0.0f;
    }
    return pressure;
}

static const char *BP_StateToText(BP_State_t state)
{
    switch (state)
    {
    case BP_STATE_IDLE:          return "IDLE";
    case BP_STATE_WAIT_INFLATE:  return "WAIT_INFLATE";
    case BP_STATE_INFLATING:     return "INFLATING";
    case BP_STATE_READY_DEFLATE: return "READY_DEFLATE";
    case BP_STATE_DEFLATING:     return "DEFLATING";
    case BP_STATE_CALC:          return "CALC";
    case BP_STATE_DONE:          return "DONE";
    case BP_STATE_ERR:           return "ERR";
    default:                     return "ERR";
    }
}

static float BP_Abs(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void BP_AddWavePoint(float pressure, float amp, uint32_t now)
{
    if (bp.waveCount >= BP_WAVE_POINT_MAX)
    {
        return;
    }

    if ((bp.lastPeakTick != 0U) && ((now - bp.lastPeakTick) < BP_PEAK_REFRACTORY_MS))
    {
        return;
    }

    if (amp < BP_MIN_OSC_AMP)
    {
        return;
    }

    bp.wave[bp.waveCount].press_mmhg = pressure;
    bp.wave[bp.waveCount].amp = amp;
    bp.wave[bp.waveCount].tick_ms = now;
    bp.waveCount++;
    bp.lastPeakTick = now;
}

static void BP_UpdateOscillation(float pressure, uint32_t now)
{
    float osc;
    float absOsc;

    bp.oscDc = (0.98f * bp.oscDc) + (0.02f * pressure);
    osc = pressure - bp.oscDc;
    absOsc = BP_Abs(osc);

    if ((bp.prevAbsOsc > absOsc) && (bp.prevAbsOsc > BP_MIN_OSC_AMP))
    {
        BP_AddWavePoint(pressure, bp.prevAbsOsc, now);
    }

    bp.prevAbsOsc = absOsc;
}

static float BP_FindThresholdPressure(uint16_t start, int8_t direction, float threshold)
{
    int16_t i = (int16_t)start;
    float bestPressure = bp.wave[start].press_mmhg;
    float bestDiff = BP_Abs(bp.wave[start].amp - threshold);

    while ((i >= 0) && (i < (int16_t)bp.waveCount))
    {
        float diff = BP_Abs(bp.wave[i].amp - threshold);
        if (diff < bestDiff)
        {
            bestDiff = diff;
            bestPressure = bp.wave[i].press_mmhg;
        }

        if (bp.wave[i].amp <= threshold)
        {
            return bp.wave[i].press_mmhg;
        }

        i = (int16_t)(i + direction);
    }

    return bestPressure;
}

static void BP_CalculateResult(void)
{
    uint16_t i;
    uint16_t mapIndex = 0U;
    float maxAmp = 0.0f;
    uint32_t firstTick;
    uint32_t lastTick;

    if (bp.waveCount < BP_MIN_VALID_POINTS)
    {
        BP_DEBUG_PRINTF("[BP] fail reason=POINTS wave=%u min=%u\r\n",
                        bp.waveCount,
                        BP_MIN_VALID_POINTS);
        BP_SetState(BP_STATE_ERR, "points");
        return;
    }

    for (i = 0U; i < bp.waveCount; i++)
    {
        if (bp.wave[i].amp > maxAmp)
        {
            maxAmp = bp.wave[i].amp;
            mapIndex = i;
        }
    }

    if (maxAmp <= 0.0f)
    {
        BP_DEBUG_PRINTF("[BP] fail reason=NO_AMP wave=%u\r\n", bp.waveCount);
        BP_SetState(BP_STATE_ERR, "no_amp");
        return;
    }

    bp.map = bp.wave[mapIndex].press_mmhg;
    bp.sbp = BP_FindThresholdPressure(mapIndex, -1, maxAmp * BP_SBP_COEFF);
    bp.dbp = BP_FindThresholdPressure(mapIndex, 1, maxAmp * BP_DBP_COEFF);

    firstTick = bp.wave[0].tick_ms;
    lastTick = bp.wave[bp.waveCount - 1U].tick_ms;
    if (lastTick > firstTick)
    {
        float minutes = ((float)(lastTick - firstTick)) / 60000.0f;
        bp.hr = (uint8_t)(((float)(bp.waveCount - 1U) / minutes) + 0.5f);
    }
    else
    {
        bp.hr = 0U;
    }

    if ((bp.hr < BP_VALID_HR_MIN) && (bp.waveCount >= BP_MIN_VALID_POINTS))
    {
        bp.hr = BP_VALID_HR_MIN;
    }
    else if (bp.hr > BP_VALID_HR_MAX)
    {
        bp.hr = BP_VALID_HR_MAX;
    }

    if ((bp.sbp > bp.dbp) &&
        (bp.sbp > BP_VALID_SBP_MIN) && (bp.sbp < BP_VALID_SBP_MAX) &&
        (bp.dbp > BP_VALID_DBP_MIN) && (bp.dbp < BP_VALID_DBP_MAX) &&
        (bp.map > BP_VALID_MAP_MIN) && (bp.map < BP_VALID_MAP_MAX) &&
        (bp.hr >= BP_VALID_HR_MIN) && (bp.hr <= BP_VALID_HR_MAX))
    {
        bp.resultValid = 1U;
        BP_DEBUG_PRINTF("[BP] calc ok wave=%u maxAmp=%.2f map=%.1f sbp=%.1f dbp=%.1f hr=%u\r\n",
                        bp.waveCount,
                        maxAmp,
                        bp.map,
                        bp.sbp,
                        bp.dbp,
                        bp.hr);
        BP_SetState(BP_STATE_DONE, "calc_ok");
        BP_StopPressureSampling();
    }
    else
    {
        bp.resultValid = 0U;
        BP_DEBUG_PRINTF("[BP] fail reason=RANGE wave=%u maxAmp=%.2f map=%.1f sbp=%.1f dbp=%.1f hr=%u\r\n",
                        bp.waveCount,
                        maxAmp,
                        bp.map,
                        bp.sbp,
                        bp.dbp,
                        bp.hr);
        BP_SetState(BP_STATE_ERR, "range");
        BP_StopPressureSampling();
    }
}

static void BP_ProcessPressure(float pressure, uint32_t now)
{
    float delta;

    if (bp.filteredPressure <= 0.1f)
    {
        bp.filteredPressure = pressure;
    }
    else
    {
        bp.filteredPressure = (0.85f * bp.filteredPressure) + (0.15f * pressure);
    }

    bp.pressure = bp.filteredPressure;

    if ((now - bp.lastSpeedTick) >= BP_SPEED_INTERVAL_MS)
    {
        float dt = ((float)(now - bp.lastSpeedTick)) / 1000.0f;
        bp.deflateSpeed = (bp.lastSpeedPressure - bp.pressure) / dt;
        bp.lastSpeedPressure = bp.pressure;
        bp.lastSpeedTick = now;
    }

    switch (bp.state)
    {
    case BP_STATE_WAIT_INFLATE:
        delta = bp.pressure - bp.lastSpeedPressure;
        if ((bp.pressure > 20.0f) || (delta > 1.0f))
        {
            BP_SetState(BP_STATE_INFLATING, "pressure_rise");
        }
        break;

    case BP_STATE_INFLATING:
        if ((bp.pressure >= BP_TARGET_PRESS_LOW) && (bp.pressure <= BP_TARGET_PRESS_HIGH))
        {
            BP_SetState(BP_STATE_READY_DEFLATE, "target");
        }
        else if (bp.pressure > (BP_TARGET_PRESS_HIGH + 25.0f))
        {
            BP_DEBUG_PRINTF("[BP] fail reason=OVER_PRESS pressure=%.1f limit=%.1f\r\n",
                            bp.pressure,
                            BP_TARGET_PRESS_HIGH + 25.0f);
            BP_SetState(BP_STATE_ERR, "over_press");
            BP_StopPressureSampling();
        }
        break;

    case BP_STATE_READY_DEFLATE:
        if (bp.deflateSpeed > 0.5f)
        {
            BP_SetState(BP_STATE_DEFLATING, "deflate_start");
            bp.waveCount = 0U;
            bp.oscDc = bp.pressure;
            bp.prevAbsOsc = 0.0f;
            bp.lastPeakTick = 0U;
        }
        break;

    case BP_STATE_DEFLATING:
        BP_UpdateOscillation(bp.pressure, now);
        if (bp.pressure <= BP_STOP_PRESS)
        {
            BP_SetState(BP_STATE_CALC, "stop_pressure");
            BP_CalculateResult();
        }
        break;

    default:
        break;
    }
}

void BP_Service_Init(void)
{
    BP_Service_Reset();
    BP_LoadDefaultCalibration();
}

void BP_Service_Start(void)
{
    BP_Service_Reset();
    SensorService_SetPressureRunning(1U);
    BP_SetState(BP_STATE_WAIT_INFLATE, "start");
    bp.lastSpeedTick = HAL_GetTick();
    bp.lastSpeedPressure = bp.pressure;
}

void BP_Service_Reset(void)
{
    uint8_t stopSampling = (bp.state != BP_STATE_IDLE) ? 1U : 0U;
    uint16_t calAdc1 = bp.calAdc1;
    uint16_t calAdc2 = bp.calAdc2;
    float calMmHg1 = bp.calMmHg1;
    float calMmHg2 = bp.calMmHg2;

    memset(&bp, 0, sizeof(bp));
    bp.state = BP_STATE_IDLE;
    bp.calAdc1 = calAdc1;
    bp.calAdc2 = calAdc2;
    bp.calMmHg1 = calMmHg1;
    bp.calMmHg2 = calMmHg2;

    if (bp.calAdc1 == bp.calAdc2)
    {
        BP_LoadDefaultCalibration();
    }

    if (stopSampling != 0U)
    {
        BP_StopPressureSampling();
    }
}

void BP_Service_SetCalibration(uint16_t adc1, float mmHg1, uint16_t adc2, float mmHg2)
{
    int32_t adcDelta = (int32_t)adc2 - (int32_t)adc1;
    float mmHgDelta = mmHg2 - mmHg1;

    if (((adcDelta > -4) && (adcDelta < 4)) ||
        ((mmHgDelta > -1.0f) && (mmHgDelta < 1.0f)))
    {
        return;
    }

    bp.calAdc1 = adc1;
    bp.calMmHg1 = mmHg1;
    bp.calAdc2 = adc2;
    bp.calMmHg2 = mmHg2;
}

void BP_Service_ResetCalibration(void)
{
    BP_LoadDefaultCalibration();
}

void BP_Service_Tick(void)
{
    SensorData_t data;
    uint32_t now = HAL_GetTick();

    if ((bp.state == BP_STATE_IDLE) || (bp.state == BP_STATE_DONE) || (bp.state == BP_STATE_ERR))
    {
        return;
    }

    SensorService_GetData(&data);
    if ((bp.hasAdc == 0U) || (data.pressureRaw != bp.lastAdc))
    {
        bp.lastAdc = data.pressureRaw;
        bp.hasAdc = 1U;
        BP_ProcessPressure(BP_Service_CalcPressureFromAdc(data.pressureRaw), now);
    }
}

void BP_Service_OnAdcSamples(const uint16_t *buf, uint16_t len)
{
    uint32_t sum = 0U;
    uint16_t i;

    if ((buf == 0) || (len == 0U))
    {
        return;
    }

    for (i = 0U; i < len; i++)
    {
        sum += buf[i];
    }

    bp.lastAdc = (uint16_t)(sum / len);
    bp.hasAdc = 1U;
    if ((bp.state != BP_STATE_IDLE) && (bp.state != BP_STATE_DONE) && (bp.state != BP_STATE_ERR))
    {
        BP_ProcessPressure(BP_Service_CalcPressureFromAdc(bp.lastAdc), HAL_GetTick());
    }
}

float BP_Service_GetPressure(void)
{
    return bp.pressure;
}

float BP_Service_GetDeflateSpeed(void)
{
    return (bp.deflateSpeed > 0.0f) ? bp.deflateSpeed : 0.0f;
}

float BP_Service_GetSBP(void)
{
    return bp.sbp;
}

float BP_Service_GetDBP(void)
{
    return bp.dbp;
}

float BP_Service_GetMAP(void)
{
    return bp.map;
}

uint8_t BP_Service_GetHR(void)
{
    return bp.hr;
}

uint8_t BP_Service_IsResultValid(void)
{
    return bp.resultValid;
}

BP_State_t BP_Service_GetState(void)
{
    return bp.state;
}

const char* BP_Service_GetHintText(void)
{
    switch (bp.state)
    {
    case BP_STATE_IDLE:
        return "Press START";
    case BP_STATE_WAIT_INFLATE:
        return "Inflate manually";
    case BP_STATE_INFLATING:
        return "Keep inflating";
    case BP_STATE_READY_DEFLATE:
        return "Stop inflating and deflate slowly";
    case BP_STATE_DEFLATING:
        if (bp.deflateSpeed < BP_DEFLATE_SPEED_MIN)
        {
            return "Deflate faster";
        }
        if (bp.deflateSpeed > BP_DEFLATE_SPEED_MAX)
        {
            return "Deflate slower";
        }
        return "Good speed";
    case BP_STATE_CALC:
        return "Calculating";
    case BP_STATE_DONE:
        return "Done";
    case BP_STATE_ERR:
        return "Measure failed";
    default:
        return "Measure failed";
    }
}

const char* BP_Service_GetStateText(void)
{
    return BP_StateToText(bp.state);
}
