#ifndef APP_BUZZER_SERVICE_H
#define APP_BUZZER_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    APP_BUZZER_PATTERN_CLICK = 0,
    APP_BUZZER_PATTERN_SUCCESS,
    APP_BUZZER_PATTERN_WARNING,
    APP_BUZZER_PATTERN_CRITICAL,
    APP_BUZZER_PATTERN_CUSTOM
} AppBuzzerPattern_t;

void AppBuzzerService_Init(void);
void AppBuzzerService_Task(void *argument);
void AppBuzzer_SetOutput(uint8_t on);
void AppBuzzer_Play(AppBuzzerPattern_t pattern);
void AppBuzzer_Beep(uint16_t durationMs);
void AppBuzzer_SetMuted(uint8_t muted);
uint8_t AppBuzzer_IsMuted(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_BUZZER_SERVICE_H */
