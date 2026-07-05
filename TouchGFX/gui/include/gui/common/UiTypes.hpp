#ifndef UITYPES_HPP
#define UITYPES_HPP

#include <stdint.h>

static const uint8_t GUI_RTC_TX_FRAME_SIZE = 10U;

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} GuiRtcDateTime;

typedef struct
{
    uint8_t ready;
    uint8_t usingLse;
    uint8_t backupValid;
} GuiRtcStatus;

#endif // UITYPES_HPP
