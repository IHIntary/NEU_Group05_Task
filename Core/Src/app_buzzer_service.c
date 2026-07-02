#include "app_buzzer_service.h"
#include "app_i2c_bus.h"
#include "cmsis_os2.h"

#define APP_BUZZER_I2C_ADDR        0x40U
#define APP_BUZZER_I2C_TIMEOUT_MS  50U
#define APP_BUZZER_QUEUE_LENGTH    8U

typedef struct
{
    AppBuzzerPattern_t pattern;
    uint16_t customDurationMs;
} AppBuzzerRequest_t;

static osMessageQueueId_t Group05_BuzzerQ = NULL;
static uint8_t buzzerMuted = 0U;

static const osMessageQueueAttr_t Group05_BuzzerQ_attributes = {
    .name = "Group05_BuzzerQ",
};

static void AppBuzzer_SetOutput(uint8_t on)
{
    uint8_t value = on ? 0x00U : 0x01U;
    (void)AppI2c2_MasterTransmit(APP_BUZZER_I2C_ADDR, &value, 1U, APP_BUZZER_I2C_TIMEOUT_MS);
}

static void AppBuzzer_BeepBlocking(uint16_t onMs)
{
    if (onMs == 0U)
    {
        return;
    }

    AppBuzzer_SetOutput(1U);
    osDelay(onMs);
    AppBuzzer_SetOutput(0U);
}

static void AppBuzzer_PlaySequence(AppBuzzerPattern_t pattern, uint16_t customDurationMs)
{
    switch (pattern)
    {
    case APP_BUZZER_PATTERN_CLICK:
        AppBuzzer_BeepBlocking(60U);
        break;

    case APP_BUZZER_PATTERN_SUCCESS:
        AppBuzzer_BeepBlocking(80U);
        osDelay(80U);
        AppBuzzer_BeepBlocking(80U);
        break;

    case APP_BUZZER_PATTERN_WARNING:
        AppBuzzer_BeepBlocking(90U);
        osDelay(90U);
        AppBuzzer_BeepBlocking(90U);
        break;

    case APP_BUZZER_PATTERN_CRITICAL:
        AppBuzzer_BeepBlocking(140U);
        osDelay(80U);
        AppBuzzer_BeepBlocking(140U);
        osDelay(80U);
        AppBuzzer_BeepBlocking(140U);
        break;

    case APP_BUZZER_PATTERN_CUSTOM:
    default:
        AppBuzzer_BeepBlocking(customDurationMs);
        break;
    }

    AppBuzzer_SetOutput(0U);
}

static uint8_t AppBuzzer_GetPriority(AppBuzzerPattern_t pattern)
{
    switch (pattern)
    {
    case APP_BUZZER_PATTERN_CRITICAL:
        return 3U;
    case APP_BUZZER_PATTERN_WARNING:
        return 2U;
    case APP_BUZZER_PATTERN_SUCCESS:
        return 1U;
    case APP_BUZZER_PATTERN_CLICK:
    case APP_BUZZER_PATTERN_CUSTOM:
    default:
        return 0U;
    }
}

void AppBuzzerService_Init(void)
{
    if (Group05_BuzzerQ == NULL)
    {
        Group05_BuzzerQ = osMessageQueueNew(APP_BUZZER_QUEUE_LENGTH,
                                            sizeof(AppBuzzerRequest_t),
                                            &Group05_BuzzerQ_attributes);
    }

    buzzerMuted = 0U;
}

void AppBuzzer_Play(AppBuzzerPattern_t pattern)
{
    AppBuzzerRequest_t request;

    if ((Group05_BuzzerQ == NULL) || (buzzerMuted != 0U))
    {
        return;
    }

    request.pattern = pattern;
    request.customDurationMs = 0U;
    (void)osMessageQueuePut(Group05_BuzzerQ,
                            &request,
                            AppBuzzer_GetPriority(pattern),
                            0U);
}

void AppBuzzer_Beep(uint16_t durationMs)
{
    AppBuzzerRequest_t request;

    if ((Group05_BuzzerQ == NULL) || (buzzerMuted != 0U))
    {
        return;
    }

    request.pattern = APP_BUZZER_PATTERN_CUSTOM;
    request.customDurationMs = durationMs;
    (void)osMessageQueuePut(Group05_BuzzerQ, &request, 0U, 0U);
}

void AppBuzzer_SetMuted(uint8_t muted)
{
    buzzerMuted = muted ? 1U : 0U;
    if (buzzerMuted != 0U)
    {
        AppBuzzer_SetOutput(0U);
    }
}

uint8_t AppBuzzer_IsMuted(void)
{
    return buzzerMuted;
}

void AppBuzzerService_Task(void *argument)
{
    AppBuzzerRequest_t request;

    (void)argument;
    AppBuzzer_SetOutput(0U);

    for (;;)
    {
        if (Group05_BuzzerQ == NULL)
        {
            osDelay(100U);
            continue;
        }

        if (osMessageQueueGet(Group05_BuzzerQ, &request, NULL, osWaitForever) == osOK)
        {
            if (buzzerMuted == 0U)
            {
                AppBuzzer_PlaySequence(request.pattern, request.customDurationMs);
            }
        }
    }
}
