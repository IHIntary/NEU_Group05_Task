#include <gui/clockscreen_screen/ClockScreenView.hpp>

ClockScreenView::ClockScreenView()
    : selectedField(FIELD_YEAR),
      digitalMode(1U),
      editInitialized(0U)
{
    editTime.year = 2026U;
    editTime.month = 1U;
    editTime.day = 1U;
    editTime.hour = 0U;
    editTime.minute = 0U;
    editTime.second = 0U;
    lastRtcTime = editTime;
}

void ClockScreenView::setupScreen()
{
    ClockScreenViewBase::setupScreen();

    timeText.setWildcard(timeBuffer);
    dateText.setWildcard(dateBuffer);
    statusText.setWildcard(statusBuffer);
    editText.setWildcard(editBuffer);
    fieldText.setWildcard(fieldBuffer);
    txText.setWildcard(txBuffer);

    touchgfx::Unicode::snprintf(timeBuffer, TIME_TEXT_SIZE, "00:00:00");
    touchgfx::Unicode::snprintf(dateBuffer, DATE_TEXT_SIZE, "2026-01-01");
    touchgfx::Unicode::snprintf(statusBuffer, STATUS_TEXT_SIZE, "RTC INIT");
    touchgfx::Unicode::snprintf(txBuffer, TX_TEXT_SIZE, "--");

    presenter->getRtcDateTime(lastRtcTime);
    syncEditFromRtc(lastRtcTime);
    updateAnalogHands(lastRtcTime);
    setClockStyle(1U);
}

void ClockScreenView::tearDownScreen()
{
    ClockScreenViewBase::tearDownScreen();
}

void ClockScreenView::styleClicked()
{
    setClockStyle((digitalMode == 0U) ? 1U : 0U);
}

void ClockScreenView::fieldClicked()
{
    selectNextField();
}

void ClockScreenView::downClicked()
{
    adjustSelectedField(-1);
}

void ClockScreenView::upClicked()
{
    adjustSelectedField(1);
}

void ClockScreenView::setClicked()
{
    if (presenter->setRtcDateTime(editTime) != 0U)
    {
        sendHexFrame(editTime);
    }
}

void ClockScreenView::syncClicked()
{
    presenter->getRtcDateTime(lastRtcTime);
    syncEditFromRtc(lastRtcTime);
}

void ClockScreenView::updateRtc(const AppRtcDateTime_t& dateTime, const AppRtcStatus_t& status)
{
    lastRtcTime = dateTime;

    if (editInitialized == 0U)
    {
        syncEditFromRtc(dateTime);
    }

    touchgfx::Unicode::snprintf(timeBuffer, TIME_TEXT_SIZE, "%02u:%02u:%02u",
                                dateTime.hour, dateTime.minute, dateTime.second);
    touchgfx::Unicode::snprintf(dateBuffer, DATE_TEXT_SIZE, "%04u-%02u-%02u",
                                dateTime.year, dateTime.month, dateTime.day);

    if (status.ready == 0U)
    {
        touchgfx::Unicode::snprintf(statusBuffer, STATUS_TEXT_SIZE, "RTC INIT FAIL");
    }
    else
    {
        touchgfx::Unicode::snprintf(statusBuffer, STATUS_TEXT_SIZE,
                                    (status.usingLse != 0U) ? "RTC OK LSE" : "LSE FAIL");
    }

    updateAnalogHands(dateTime);
    timeText.invalidate();
    dateText.invalidate();
    statusText.invalidate();
}

void ClockScreenView::syncEditFromRtc(const AppRtcDateTime_t& dateTime)
{
    editTime = dateTime;
    editInitialized = 1U;
    updateEditTexts();
}

void ClockScreenView::updateAnalogHands(const AppRtcDateTime_t& dateTime)
{
    analogClock.setTime24Hour(dateTime.hour, dateTime.minute, dateTime.second);
    analogClock.invalidate();
}

void ClockScreenView::setClockStyle(uint8_t digital)
{
    digitalMode = digital ? 1U : 0U;

    timeText.setVisible(digitalMode != 0U);
    dateText.setVisible(digitalMode != 0U);
    statusText.setVisible(digitalMode != 0U);
    analogClock.setVisible(digitalMode == 0U);

    timeText.invalidate();
    dateText.invalidate();
    statusText.invalidate();
    analogClock.invalidate();
}

void ClockScreenView::selectNextField()
{
    selectedField = (EditField)(((uint8_t)selectedField + 1U) % (uint8_t)FIELD_COUNT);
    updateEditTexts();
}

void ClockScreenView::adjustSelectedField(int8_t delta)
{
    switch (selectedField)
    {
    case FIELD_YEAR:
        if ((delta > 0) && (editTime.year < 2099U))
        {
            editTime.year++;
        }
        else if ((delta < 0) && (editTime.year > 2000U))
        {
            editTime.year--;
        }
        break;
    case FIELD_MONTH:
        editTime.month = (uint8_t)((((int16_t)editTime.month - 1) + delta + 12) % 12 + 1);
        break;
    case FIELD_DAY:
        {
            uint8_t maxDay = daysInMonth(editTime.year, editTime.month);
            editTime.day = (uint8_t)((((int16_t)editTime.day - 1) + delta + maxDay) % maxDay + 1);
        }
        break;
    case FIELD_HOUR:
        editTime.hour = (uint8_t)(((int16_t)editTime.hour + delta + 24) % 24);
        break;
    case FIELD_MINUTE:
        editTime.minute = (uint8_t)(((int16_t)editTime.minute + delta + 60) % 60);
        break;
    case FIELD_SECOND:
        editTime.second = (uint8_t)(((int16_t)editTime.second + delta + 60) % 60);
        break;
    default:
        break;
    }

    if (editTime.day > daysInMonth(editTime.year, editTime.month))
    {
        editTime.day = daysInMonth(editTime.year, editTime.month);
    }

    updateEditTexts();
}

void ClockScreenView::updateEditTexts()
{
    static const char* const fieldNames[] = {
        "YEAR",
        "MONTH",
        "DAY",
        "HOUR",
        "MIN",
        "SEC"
    };

    touchgfx::Unicode::snprintf(editBuffer, EDIT_TEXT_SIZE, "%04u-%02u-%02u %02u:%02u:%02u",
                                editTime.year, editTime.month, editTime.day,
                                editTime.hour, editTime.minute, editTime.second);
    touchgfx::Unicode::snprintf(fieldBuffer, FIELD_TEXT_SIZE, fieldNames[(uint8_t)selectedField]);

    editText.invalidate();
    fieldText.invalidate();
}

void ClockScreenView::sendHexFrame(const AppRtcDateTime_t& dateTime)
{
    uint8_t frame[APP_RTC_TX_FRAME_SIZE];
    uint16_t pos = 0U;

    if (presenter->sendRtcDateTimeHex(dateTime) == 0U)
    {
        touchgfx::Unicode::snprintf(txBuffer, TX_TEXT_SIZE, "TX FAIL");
        txText.invalidate();
        return;
    }

    AppRtcService_BuildFrame(&dateTime, frame, APP_RTC_TX_FRAME_SIZE);
    for (uint8_t i = 0U; i < APP_RTC_TX_FRAME_SIZE; i++)
    {
        if (pos < (TX_TEXT_SIZE - 3U))
        {
            touchgfx::Unicode::snprintf(&txBuffer[pos], (uint16_t)(TX_TEXT_SIZE - pos), "%02X", frame[i]);
            pos = (uint16_t)(pos + 2U);
        }
        if ((i < (APP_RTC_TX_FRAME_SIZE - 1U)) && (pos < (TX_TEXT_SIZE - 1U)))
        {
            txBuffer[pos++] = ' ';
            txBuffer[pos] = 0;
        }
    }

    txText.invalidate();
}

uint8_t ClockScreenView::isLeapYear(uint16_t year) const
{
    return (((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U))) ? 1U : 0U;
}

uint8_t ClockScreenView::daysInMonth(uint16_t year, uint8_t month) const
{
    static const uint8_t days[12] = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };

    if ((month < 1U) || (month > 12U))
    {
        return 31U;
    }

    if ((month == 2U) && (isLeapYear(year) != 0U))
    {
        return 29U;
    }

    return days[month - 1U];
}
