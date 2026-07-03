#ifndef CLOCKSCREENVIEW_HPP
#define CLOCKSCREENVIEW_HPP

#include <gui_generated/clockscreen_screen/ClockScreenViewBase.hpp>
#include <gui/clockscreen_screen/ClockScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>
#include <stdint.h>

class ClockScreenView : public ClockScreenViewBase
{
public:
    ClockScreenView();
    virtual ~ClockScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void styleClicked();
    virtual void fieldClicked();
    virtual void downClicked();
    virtual void upClicked();
    virtual void setClicked();
    virtual void syncClicked();

    void updateRtc(const AppRtcDateTime_t& dateTime, const AppRtcStatus_t& status);

protected:
    static const uint16_t TIME_TEXT_SIZE = 16;
    static const uint16_t DATE_TEXT_SIZE = 16;
    static const uint16_t EDIT_TEXT_SIZE = 24;
    static const uint16_t FIELD_TEXT_SIZE = 12;
    static const uint16_t TX_TEXT_SIZE = 32;
    static const uint16_t STATUS_TEXT_SIZE = 20;

private:
    enum EditField
    {
        FIELD_YEAR = 0,
        FIELD_MONTH,
        FIELD_DAY,
        FIELD_HOUR,
        FIELD_MINUTE,
        FIELD_SECOND,
        FIELD_COUNT
    };

    void syncEditFromRtc(const AppRtcDateTime_t& dateTime);
    void updateEditTexts();
    void updateAnalogHands(const AppRtcDateTime_t& dateTime);
    void setClockStyle(uint8_t digital);
    void selectNextField();
    void adjustSelectedField(int8_t delta);
    void sendHexFrame(const AppRtcDateTime_t& dateTime);
    uint8_t daysInMonth(uint16_t year, uint8_t month) const;
    uint8_t isLeapYear(uint16_t year) const;

    touchgfx::Unicode::UnicodeChar timeBuffer[TIME_TEXT_SIZE];
    touchgfx::Unicode::UnicodeChar dateBuffer[DATE_TEXT_SIZE];
    touchgfx::Unicode::UnicodeChar editBuffer[EDIT_TEXT_SIZE];
    touchgfx::Unicode::UnicodeChar fieldBuffer[FIELD_TEXT_SIZE];
    touchgfx::Unicode::UnicodeChar txBuffer[TX_TEXT_SIZE];
    touchgfx::Unicode::UnicodeChar statusBuffer[STATUS_TEXT_SIZE];

    AppRtcDateTime_t editTime;
    AppRtcDateTime_t lastRtcTime;
    AppRtcDateTime_t displayedRtcTime;
    AppRtcStatus_t displayedRtcStatus;
    EditField selectedField;
    uint8_t digitalMode;
    uint8_t editInitialized;
    uint8_t rtcDisplayValid;
};

#endif // CLOCKSCREENVIEW_HPP
