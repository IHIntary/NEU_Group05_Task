#include "app_rtc_service.h"
#include "stm32f4xx_hal.h"
#include "usart.h"

#define APP_RTC_BACKUP_MAGIC        0xA55A2026UL
#define APP_RTC_LSE_TIMEOUT_MS      2000U
#define APP_RTC_TX_ASCII_SIZE       ((APP_RTC_TX_FRAME_SIZE * 3U) + 2U)

static RTC_HandleTypeDef hrtcApp;
static AppRtcStatus_t rtcStatus;

static uint8_t AppRtc_IsLeapYear(uint16_t year)
{
    return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U))) ? 1U : 0U;
}

static uint8_t AppRtc_DaysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };

    if ((month < 1U) || (month > 12U))
    {
        return 31U;
    }

    if ((month == 2U) && (AppRtc_IsLeapYear(year) != 0U))
    {
        return 29U;
    }

    return days[month - 1U];
}

static uint8_t AppRtc_IsDateTimeValid(const AppRtcDateTime_t *dateTime)
{
    if (dateTime == 0)
    {
        return 0U;
    }

    if ((dateTime->year < 2000U) || (dateTime->year > 2099U) ||
        (dateTime->month < 1U) || (dateTime->month > 12U) ||
        (dateTime->day < 1U) || (dateTime->day > AppRtc_DaysInMonth(dateTime->year, dateTime->month)) ||
        (dateTime->hour > 23U) || (dateTime->minute > 59U) || (dateTime->second > 59U))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t AppRtc_WeekDay(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t y = year;
    uint8_t m = month;
    uint16_t k;
    uint16_t j;
    uint16_t h;

    if (m < 3U)
    {
        m += 12U;
        y--;
    }

    k = (uint16_t)(y % 100U);
    j = (uint16_t)(y / 100U);
    h = (uint16_t)((day + ((13U * (m + 1U)) / 5U) + k + (k / 4U) + (j / 4U) + (5U * j)) % 7U);

    if (h == 0U)
    {
        return RTC_WEEKDAY_SATURDAY;
    }

    if (h == 1U)
    {
        return RTC_WEEKDAY_SUNDAY;
    }

    return (uint8_t)(h - 1U);
}

static uint8_t AppRtc_WaitForFlag(volatile uint32_t *reg, uint32_t mask, uint32_t timeoutMs)
{
    uint32_t start = HAL_GetTick();

    while (((*reg) & mask) == 0U)
    {
        if ((HAL_GetTick() - start) >= timeoutMs)
        {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t AppRtc_SelectClockSource(uint8_t resetBackupDomain)
{
    if ((resetBackupDomain == 0U) && ((RCC->BDCR & RCC_BDCR_RTCSEL) == RCC_BDCR_RTCSEL_0))
    {
        RCC->BDCR |= RCC_BDCR_LSEON;
        if (AppRtc_WaitForFlag(&RCC->BDCR, RCC_BDCR_LSERDY, APP_RTC_LSE_TIMEOUT_MS) == 0U)
        {
            return 0U;
        }
        RCC->BDCR |= RCC_BDCR_RTCEN;
        return 1U;
    }

    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();

    RCC->BDCR |= RCC_BDCR_LSEON;
    if (AppRtc_WaitForFlag(&RCC->BDCR, RCC_BDCR_LSERDY, APP_RTC_LSE_TIMEOUT_MS) == 0U)
    {
        return 0U;
    }

    MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_0);
    RCC->BDCR |= RCC_BDCR_RTCEN;
    return 1U;
}

static void AppRtc_SetHandlePrescalers(void)
{
    hrtcApp.Instance = RTC;
    hrtcApp.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtcApp.Init.AsynchPrediv = 127U;
    hrtcApp.Init.SynchPrediv = 255U;
    hrtcApp.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtcApp.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtcApp.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
}

void AppRtcService_Init(void)
{
    uint8_t backupValid;
    uint8_t useLse;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    backupValid = (RTC->BKP0R == APP_RTC_BACKUP_MAGIC) ? 1U : 0U;
    if ((RCC->BDCR & RCC_BDCR_RTCSEL) != RCC_BDCR_RTCSEL_0)
    {
        backupValid = 0U;
    }

    useLse = AppRtc_SelectClockSource((backupValid == 0U) ? 1U : 0U);
    AppRtc_SetHandlePrescalers();

    rtcStatus.ready = ((useLse != 0U) && (HAL_RTC_Init(&hrtcApp) == HAL_OK)) ? 1U : 0U;
    rtcStatus.usingLse = useLse;
    rtcStatus.backupValid = (useLse != 0U) ? backupValid : 0U;

    if ((rtcStatus.ready != 0U) && (backupValid == 0U))
    {
        AppRtcDateTime_t defaultTime;
        defaultTime.year = 2026U;
        defaultTime.month = 1U;
        defaultTime.day = 1U;
        defaultTime.hour = 0U;
        defaultTime.minute = 0U;
        defaultTime.second = 0U;

        (void)AppRtcService_SetDateTime(&defaultTime);
        RTC->BKP0R = APP_RTC_BACKUP_MAGIC;
        rtcStatus.backupValid = 1U;
    }
}

void AppRtcService_GetDateTime(AppRtcDateTime_t *out)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    if (out == 0)
    {
        return;
    }

    if (rtcStatus.ready == 0U)
    {
        out->year = 2026U;
        out->month = 1U;
        out->day = 1U;
        out->hour = 0U;
        out->minute = 0U;
        out->second = 0U;
        return;
    }

    if ((HAL_RTC_GetTime(&hrtcApp, &time, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_GetDate(&hrtcApp, &date, RTC_FORMAT_BIN) != HAL_OK))
    {
        return;
    }

    out->year = (uint16_t)(2000U + date.Year);
    out->month = date.Month;
    out->day = date.Date;
    out->hour = time.Hours;
    out->minute = time.Minutes;
    out->second = time.Seconds;
}

uint8_t AppRtcService_SetDateTime(const AppRtcDateTime_t *dateTime)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    if ((rtcStatus.ready == 0U) || (AppRtc_IsDateTimeValid(dateTime) == 0U))
    {
        return 0U;
    }

    time.Hours = dateTime->hour;
    time.Minutes = dateTime->minute;
    time.Seconds = dateTime->second;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.Year = (uint8_t)(dateTime->year - 2000U);
    date.Month = dateTime->month;
    date.Date = dateTime->day;
    date.WeekDay = AppRtc_WeekDay(dateTime->year, dateTime->month, dateTime->day);

    if ((HAL_RTC_SetTime(&hrtcApp, &time, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_SetDate(&hrtcApp, &date, RTC_FORMAT_BIN) != HAL_OK))
    {
        return 0U;
    }

    RTC->BKP0R = APP_RTC_BACKUP_MAGIC;
    rtcStatus.backupValid = 1U;
    return 1U;
}

void AppRtcService_GetStatus(AppRtcStatus_t *out)
{
    if (out == 0)
    {
        return;
    }

    *out = rtcStatus;
}

void AppRtcService_BuildFrame(const AppRtcDateTime_t *dateTime, uint8_t *frame, uint8_t frameLen)
{
    uint8_t i;
    uint8_t checksum = 0U;

    if ((dateTime == 0) || (frame == 0) || (frameLen < APP_RTC_TX_FRAME_SIZE))
    {
        return;
    }

    frame[0] = 0xA5U;
    frame[1] = 0x5AU;
    frame[2] = 0x06U;
    frame[3] = (uint8_t)(dateTime->year - 2000U);
    frame[4] = dateTime->month;
    frame[5] = dateTime->day;
    frame[6] = dateTime->hour;
    frame[7] = dateTime->minute;
    frame[8] = dateTime->second;

    for (i = 0U; i < 9U; i++)
    {
        checksum = (uint8_t)(checksum + frame[i]);
    }
    frame[9] = checksum;
}

uint8_t AppRtcService_SendDateTimeHex(const AppRtcDateTime_t *dateTime)
{
    uint8_t frame[APP_RTC_TX_FRAME_SIZE];
    uint8_t ascii[APP_RTC_TX_ASCII_SIZE];
    static const uint8_t hexDigits[] = "0123456789ABCDEF";
    uint8_t pos = 0U;
    uint8_t i;

    if (AppRtc_IsDateTimeValid(dateTime) == 0U)
    {
        return 0U;
    }

    AppRtcService_BuildFrame(dateTime, frame, APP_RTC_TX_FRAME_SIZE);

    for (i = 0U; i < APP_RTC_TX_FRAME_SIZE; i++)
    {
        ascii[pos++] = hexDigits[(frame[i] >> 4U) & 0x0FU];
        ascii[pos++] = hexDigits[frame[i] & 0x0FU];

        if (i < (APP_RTC_TX_FRAME_SIZE - 1U))
        {
            ascii[pos++] = ' ';
        }
    }

    ascii[pos++] = '\r';
    ascii[pos++] = '\n';

    return (HAL_UART_Transmit(&huart1, ascii, pos, 100U) == HAL_OK) ? 1U : 0U;
}
