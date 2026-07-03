#include "app_light_service.h"
#include "app_buzzer_service.h"
#include "adc.h"
#include "main.h"
#include "stm32f4xx_hal.h"

#define APP_LIGHT_ADC_CHANNEL          ADC_CHANNEL_1
#define APP_LIGHT_SAMPLE_PERIOD_MS     100U
#define APP_LIGHT_BLINK_PERIOD_MS      250U
#define APP_LIGHT_THRESHOLD_DEFAULT    150U
#define APP_LIGHT_THRESHOLD_MIN        0U
#define APP_LIGHT_THRESHOLD_MAX        4095U

#define APP_LIGHT_LED_ON_STATE         GPIO_PIN_RESET
#define APP_LIGHT_LED_OFF_STATE        GPIO_PIN_SET

static AppLightStatus_t lightStatus;
static uint32_t lastSampleTick;
static uint32_t lastBlinkTick;
static uint8_t alarmBlinkOn;

static uint16_t AppLight_ReadAdc(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t value = 0U;

    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        return 0U;
    }

    if (HAL_ADC_Start(&hadc1) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
        {
            value = (uint16_t)HAL_ADC_GetValue(&hadc1);
        }
        (void)HAL_ADC_Stop(&hadc1);
    }

    return value;
}

static void AppLight_SetLed0(uint8_t on)
{
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, on ? APP_LIGHT_LED_ON_STATE : APP_LIGHT_LED_OFF_STATE);
    lightStatus.led0On = on ? 1U : 0U;
}

static void AppLight_SetAlarmOutput(uint8_t on)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, on ? APP_LIGHT_LED_ON_STATE : APP_LIGHT_LED_OFF_STATE);
    AppBuzzer_SetOutput(on ? 1U : 0U);
    alarmBlinkOn = on ? 1U : 0U;
}

static void AppLight_UpdateLed0(void)
{
    if (lightStatus.autoMode != 0U)
    {
        AppLight_SetLed0((lightStatus.lightRaw < lightStatus.threshold) ? 1U : 0U);
    }
}

static void AppLight_UpdateAlarm(uint32_t now)
{
    uint8_t aboveThreshold = (lightStatus.lightRaw > lightStatus.threshold) ? 1U : 0U;

    lightStatus.alarmActive = ((lightStatus.alarmManualOn != 0U) ||
                               ((lightStatus.alarmAutoEnabled != 0U) && (aboveThreshold != 0U))) ? 1U : 0U;

    if (lightStatus.alarmActive == 0U)
    {
        if (alarmBlinkOn != 0U)
        {
            AppLight_SetAlarmOutput(0U);
        }
        return;
    }

    if ((now - lastBlinkTick) >= APP_LIGHT_BLINK_PERIOD_MS)
    {
        lastBlinkTick = now;
        AppLight_SetAlarmOutput((alarmBlinkOn == 0U) ? 1U : 0U);
    }
}

void AppLightService_Init(void)
{
    lightStatus.lightRaw = 0U;
    lightStatus.threshold = APP_LIGHT_THRESHOLD_DEFAULT;
    lightStatus.autoMode = 1U;
    lightStatus.led0On = 0U;
    lightStatus.alarmManualOn = 0U;
    lightStatus.alarmAutoEnabled = 1U;
    lightStatus.alarmActive = 0U;

    lastSampleTick = 0U;
    lastBlinkTick = 0U;
    alarmBlinkOn = 0U;

    AppLight_SetLed0(0U);
    AppLight_SetAlarmOutput(0U);
}

void AppLightService_Tick(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - lastSampleTick) >= APP_LIGHT_SAMPLE_PERIOD_MS)
    {
        lastSampleTick = now;
        lightStatus.lightRaw = AppLight_ReadAdc(APP_LIGHT_ADC_CHANNEL);
        AppLight_UpdateLed0();
    }

    AppLight_UpdateAlarm(now);
}

void AppLightService_GetStatus(AppLightStatus_t *out)
{
    if (out == 0)
    {
        return;
    }

    *out = lightStatus;
}

void AppLightService_ToggleMode(void)
{
    lightStatus.autoMode = (lightStatus.autoMode == 0U) ? 1U : 0U;
    AppLight_UpdateLed0();
}

void AppLightService_ToggleLed0Manual(void)
{
    if (lightStatus.autoMode == 0U)
    {
        AppLight_SetLed0((lightStatus.led0On == 0U) ? 1U : 0U);
    }
}

void AppLightService_ToggleAlarmManual(void)
{
    lightStatus.alarmManualOn = (lightStatus.alarmManualOn == 0U) ? 1U : 0U;
    if (lightStatus.alarmManualOn == 0U)
    {
        AppLight_SetAlarmOutput(0U);
    }
}

void AppLightService_ToggleAlarmAuto(void)
{
    lightStatus.alarmAutoEnabled = (lightStatus.alarmAutoEnabled == 0U) ? 1U : 0U;
}

void AppLightService_AdjustThreshold(int16_t delta)
{
    int32_t next = (int32_t)lightStatus.threshold + (int32_t)delta;

    if (next < (int32_t)APP_LIGHT_THRESHOLD_MIN)
    {
        next = (int32_t)APP_LIGHT_THRESHOLD_MIN;
    }
    else if (next > (int32_t)APP_LIGHT_THRESHOLD_MAX)
    {
        next = (int32_t)APP_LIGHT_THRESHOLD_MAX;
    }

    lightStatus.threshold = (uint16_t)next;
    AppLight_UpdateLed0();
}

void AppLightService_HandleKey2Pressed(void)
{
    AppLightService_ToggleLed0Manual();
}
