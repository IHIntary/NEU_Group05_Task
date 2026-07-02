#include "app_alarm_service.h"
#include "app_buzzer_service.h"
#include "stm32f4xx_hal.h"

#define ALARM_ECG_LEADS_OFF_STABLE_MS      750U
#define ALARM_PULSE_WEAK_STABLE_MS         1500U

#define ALARM_WARNING_COOLDOWN_MS          2000U
#define ALARM_CRITICAL_COOLDOWN_MS         1000U
#define ALARM_BP_SPEED_COOLDOWN_MS         1500U

#define ALARM_BP_OVER_PRESSURE_MMHG        210.0f
#define ALARM_BP_DEFLATE_SPEED_MIN         2.0f
#define ALARM_BP_DEFLATE_SPEED_MAX         3.5f
#define ALARM_BP_SPEED_VALID_MIN           0.1f

#define ALARM_SPO2_CRITICAL                90.0f
#define ALARM_SPO2_WARNING                 94.0f
#define ALARM_HR_CRITICAL_LOW              40U
#define ALARM_HR_WARNING_LOW               50U
#define ALARM_HR_WARNING_HIGH              120U
#define ALARM_HR_CRITICAL_HIGH             140U

typedef struct
{
    uint32_t ecgLeadsOffSince;
    uint32_t pulseWeakSince;
    uint32_t lastEcgWarningTick;
    uint32_t lastPulseWarningTick;
    uint32_t lastBpWarningTick;
    uint32_t lastBpCriticalTick;
    uint8_t pulseActiveSeen;
    uint8_t bpActiveSeen;
} AppAlarmContext_t;

static AppAlarmContext_t alarmContext;

static uint8_t AppAlarm_Elapsed(uint32_t now, uint32_t since, uint32_t intervalMs)
{
    return ((since == 0U) || ((now - since) >= intervalMs)) ? 1U : 0U;
}

static void AppAlarm_PlayWarning(uint32_t now, uint32_t *lastTick, uint32_t cooldownMs)
{
    if (AppAlarm_Elapsed(now, *lastTick, cooldownMs) != 0U)
    {
        *lastTick = now;
        AppBuzzer_Play(APP_BUZZER_PATTERN_WARNING);
    }
}

static void AppAlarm_PlayCritical(uint32_t now, uint32_t *lastTick, uint32_t cooldownMs)
{
    if (AppAlarm_Elapsed(now, *lastTick, cooldownMs) != 0U)
    {
        *lastTick = now;
        AppBuzzer_Play(APP_BUZZER_PATTERN_CRITICAL);
    }
}

static void AppAlarm_UpdateEcg(const SensorData_t *data, uint32_t now)
{
    if ((data->ecgRunning != 0U) && (data->ecgLeadsOff != 0U))
    {
        if (alarmContext.ecgLeadsOffSince == 0U)
        {
            alarmContext.ecgLeadsOffSince = now;
        }

        if ((now - alarmContext.ecgLeadsOffSince) >= ALARM_ECG_LEADS_OFF_STABLE_MS)
        {
            AppAlarm_PlayWarning(now,
                                 &alarmContext.lastEcgWarningTick,
                                 ALARM_WARNING_COOLDOWN_MS);
        }
    }
    else
    {
        alarmContext.ecgLeadsOffSince = 0U;
        alarmContext.lastEcgWarningTick = 0U;
    }
}

static uint8_t AppAlarm_IsPulseHrCritical(uint16_t heartRate)
{
    if (heartRate == 0U)
    {
        return 0U;
    }

    return ((heartRate < ALARM_HR_CRITICAL_LOW) ||
            (heartRate > ALARM_HR_CRITICAL_HIGH)) ? 1U : 0U;
}

static uint8_t AppAlarm_IsPulseHrWarning(uint16_t heartRate)
{
    if (heartRate == 0U)
    {
        return 0U;
    }

    return ((heartRate < ALARM_HR_WARNING_LOW) ||
            (heartRate > ALARM_HR_WARNING_HIGH)) ? 1U : 0U;
}

static uint8_t AppAlarm_IsPulseSpo2Critical(float spo2)
{
    return ((spo2 > 0.1f) && (spo2 < ALARM_SPO2_CRITICAL)) ? 1U : 0U;
}

static uint8_t AppAlarm_IsPulseSpo2Warning(float spo2)
{
    return ((spo2 >= ALARM_SPO2_CRITICAL) && (spo2 < ALARM_SPO2_WARNING)) ? 1U : 0U;
}

static void AppAlarm_UpdatePulse(const SensorData_t *data, uint32_t now)
{
    uint8_t pulseCritical;
    uint8_t pulseWarning;

    if (data->pulseRunning != 0U)
    {
        alarmContext.pulseActiveSeen = 1U;

        if (data->pulseDataValid == 0U)
        {
            if (alarmContext.pulseWeakSince == 0U)
            {
                alarmContext.pulseWeakSince = now;
            }

            if ((now - alarmContext.pulseWeakSince) >= ALARM_PULSE_WEAK_STABLE_MS)
            {
                AppAlarm_PlayWarning(now,
                                     &alarmContext.lastPulseWarningTick,
                                     ALARM_WARNING_COOLDOWN_MS);
            }
        }
        else
        {
            alarmContext.pulseWeakSince = 0U;
        }

        return;
    }

    alarmContext.pulseWeakSince = 0U;

    if ((alarmContext.pulseActiveSeen == 0U) || (data->pulseDone == 0U))
    {
        alarmContext.pulseActiveSeen = 0U;
        return;
    }

    pulseCritical = (AppAlarm_IsPulseSpo2Critical(data->spo2) != 0U) ||
                    (AppAlarm_IsPulseHrCritical(data->heartRate) != 0U);
    pulseWarning = (AppAlarm_IsPulseSpo2Warning(data->spo2) != 0U) ||
                   (AppAlarm_IsPulseHrWarning(data->heartRate) != 0U);

    if (pulseCritical != 0U)
    {
        AppBuzzer_Play(APP_BUZZER_PATTERN_CRITICAL);
    }
    else if (pulseWarning != 0U)
    {
        AppBuzzer_Play(APP_BUZZER_PATTERN_WARNING);
    }
    else
    {
        AppBuzzer_Play(APP_BUZZER_PATTERN_SUCCESS);
    }

    alarmContext.pulseActiveSeen = 0U;
}

static void AppAlarm_UpdateBp(const AppAlarmInput_t *input, uint32_t now)
{
    if ((input->bpState != BP_STATE_IDLE) &&
        (input->bpState != BP_STATE_DONE) &&
        (input->bpState != BP_STATE_ERR))
    {
        alarmContext.bpActiveSeen = 1U;

        if (input->bpPressure > ALARM_BP_OVER_PRESSURE_MMHG)
        {
            AppAlarm_PlayCritical(now,
                                  &alarmContext.lastBpCriticalTick,
                                  ALARM_CRITICAL_COOLDOWN_MS);
        }

        if (input->bpState == BP_STATE_DEFLATING)
        {
            if ((input->bpDeflateSpeed > ALARM_BP_SPEED_VALID_MIN) &&
                (input->bpDeflateSpeed < ALARM_BP_DEFLATE_SPEED_MIN))
            {
                AppAlarm_PlayWarning(now,
                                     &alarmContext.lastBpWarningTick,
                                     ALARM_BP_SPEED_COOLDOWN_MS);
            }
            else if (input->bpDeflateSpeed > ALARM_BP_DEFLATE_SPEED_MAX)
            {
                AppAlarm_PlayWarning(now,
                                     &alarmContext.lastBpWarningTick,
                                     ALARM_BP_SPEED_COOLDOWN_MS);
            }
        }

        return;
    }

    if ((input->bpState == BP_STATE_DONE) &&
        (input->bpResultValid != 0U) &&
        (alarmContext.bpActiveSeen != 0U))
    {
        AppBuzzer_Play(APP_BUZZER_PATTERN_SUCCESS);
        alarmContext.bpActiveSeen = 0U;
    }
    else if ((input->bpState == BP_STATE_ERR) && (alarmContext.bpActiveSeen != 0U))
    {
        AppBuzzer_Play(APP_BUZZER_PATTERN_WARNING);
        alarmContext.bpActiveSeen = 0U;
    }

    if (input->bpState == BP_STATE_IDLE)
    {
        alarmContext.bpActiveSeen = 0U;
    }
}

void AppAlarm_Init(void)
{
    AppAlarm_Reset();
}

void AppAlarm_Reset(void)
{
    alarmContext.ecgLeadsOffSince = 0U;
    alarmContext.pulseWeakSince = 0U;
    alarmContext.lastEcgWarningTick = 0U;
    alarmContext.lastPulseWarningTick = 0U;
    alarmContext.lastBpWarningTick = 0U;
    alarmContext.lastBpCriticalTick = 0U;
    alarmContext.pulseActiveSeen = 0U;
    alarmContext.bpActiveSeen = 0U;
}

void AppAlarm_Update(const AppAlarmInput_t *input)
{
    uint32_t now;

    if (input == 0)
    {
        return;
    }

    now = HAL_GetTick();
    AppAlarm_UpdateEcg(&input->sensor, now);
    AppAlarm_UpdatePulse(&input->sensor, now);
    AppAlarm_UpdateBp(input, now);
}
