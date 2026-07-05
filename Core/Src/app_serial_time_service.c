#include "app_serial_time_service.h"
#include "usart.h"
#include "stm32f4xx_hal.h"

#define SERIAL_TIME_FRAME_HEADER0 0xA5U
#define SERIAL_TIME_FRAME_HEADER1 0x5AU
#define SERIAL_TIME_FRAME_PAYLOAD 0x06U
#define SERIAL_TIME_FRAME_SIZE    APP_RTC_TX_FRAME_SIZE

static uint8_t rxByte;
static uint8_t frame[SERIAL_TIME_FRAME_SIZE];
static uint8_t frameIndex;
static volatile uint8_t pendingReady;
static AppRtcDateTime_t pendingDateTime;

static uint8_t SerialTime_IsLeapYear(uint16_t year)
{
    return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U))) ? 1U : 0U;
}

static uint8_t SerialTime_DaysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };

    if ((month < 1U) || (month > 12U))
    {
        return 31U;
    }

    if ((month == 2U) && (SerialTime_IsLeapYear(year) != 0U))
    {
        return 29U;
    }

    return days[month - 1U];
}

static uint8_t SerialTime_IsDateTimeValid(const AppRtcDateTime_t *dateTime)
{
    if (dateTime == 0)
    {
        return 0U;
    }

    if ((dateTime->year < 2000U) || (dateTime->year > 2099U) ||
        (dateTime->month < 1U) || (dateTime->month > 12U) ||
        (dateTime->day < 1U) || (dateTime->day > SerialTime_DaysInMonth(dateTime->year, dateTime->month)) ||
        (dateTime->hour > 23U) || (dateTime->minute > 59U) || (dateTime->second > 59U))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t SerialTime_ChecksumValid(const uint8_t *input)
{
    uint8_t checksum = 0U;
    uint8_t i;

    for (i = 0U; i < (SERIAL_TIME_FRAME_SIZE - 1U); i++)
    {
        checksum = (uint8_t)(checksum + input[i]);
    }

    return (checksum == input[SERIAL_TIME_FRAME_SIZE - 1U]) ? 1U : 0U;
}

static void SerialTime_ResetFrame(void)
{
    frameIndex = 0U;
}

static void SerialTime_AcceptFrame(const uint8_t *input)
{
    AppRtcDateTime_t dateTime;

    if ((input[0] != SERIAL_TIME_FRAME_HEADER0) ||
        (input[1] != SERIAL_TIME_FRAME_HEADER1) ||
        (input[2] != SERIAL_TIME_FRAME_PAYLOAD) ||
        (SerialTime_ChecksumValid(input) == 0U))
    {
        return;
    }

    dateTime.year = (uint16_t)(2000U + input[3]);
    dateTime.month = input[4];
    dateTime.day = input[5];
    dateTime.hour = input[6];
    dateTime.minute = input[7];
    dateTime.second = input[8];

    if (SerialTime_IsDateTimeValid(&dateTime) == 0U)
    {
        return;
    }

    pendingDateTime = dateTime;
    pendingReady = 1U;
}

void AppSerialTimeService_Init(void)
{
    SerialTime_ResetFrame();
    pendingReady = 0U;
    (void)HAL_UART_Receive_IT(&huart1, &rxByte, 1U);
}

void AppSerialTimeService_OnRxByte(uint8_t byte)
{
    if ((frameIndex == 0U) && (byte != SERIAL_TIME_FRAME_HEADER0))
    {
        return;
    }

    if ((frameIndex == 1U) && (byte != SERIAL_TIME_FRAME_HEADER1))
    {
        frameIndex = (byte == SERIAL_TIME_FRAME_HEADER0) ? 1U : 0U;
        frame[0] = SERIAL_TIME_FRAME_HEADER0;
        return;
    }

    frame[frameIndex++] = byte;

    if (frameIndex >= SERIAL_TIME_FRAME_SIZE)
    {
        SerialTime_AcceptFrame(frame);
        SerialTime_ResetFrame();
    }
}

uint8_t AppSerialTimeService_GetPendingDateTime(AppRtcDateTime_t *out)
{
    uint8_t hasPending;

    if (out == 0)
    {
        return 0U;
    }

    __disable_irq();
    hasPending = pendingReady;
    if (hasPending != 0U)
    {
        *out = pendingDateTime;
        pendingReady = 0U;
    }
    __enable_irq();

    return hasPending;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        AppSerialTimeService_OnRxByte(rxByte);
        (void)HAL_UART_Receive_IT(&huart1, &rxByte, 1U);
    }
}
