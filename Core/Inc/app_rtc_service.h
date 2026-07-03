#ifndef APP_RTC_SERVICE_H
#define APP_RTC_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_RTC_TX_FRAME_SIZE 10U

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} AppRtcDateTime_t;

typedef struct
{
    uint8_t ready;
    uint8_t usingLse;
    uint8_t backupValid;
} AppRtcStatus_t;

void AppRtcService_Init(void);
void AppRtcService_GetDateTime(AppRtcDateTime_t *out);
uint8_t AppRtcService_SetDateTime(const AppRtcDateTime_t *dateTime);
void AppRtcService_GetStatus(AppRtcStatus_t *out);
uint8_t AppRtcService_SendDateTimeHex(const AppRtcDateTime_t *dateTime);
void AppRtcService_BuildFrame(const AppRtcDateTime_t *dateTime, uint8_t *frame, uint8_t frameLen);

#ifdef __cplusplus
}
#endif

#endif /* APP_RTC_SERVICE_H */
