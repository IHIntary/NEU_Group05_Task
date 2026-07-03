#ifndef LEDSCREENVIEW_HPP
#define LEDSCREENVIEW_HPP

#include <gui_generated/ledscreen_screen/LEDScreenViewBase.hpp>
#include <gui/ledscreen_screen/LEDScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>
#include <stdint.h>

class LEDScreenView : public LEDScreenViewBase
{
public:
    LEDScreenView();
    virtual ~LEDScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void lightModeClicked();
    virtual void led0ToggleClicked();
    virtual void alarmToggleClicked();

    void updateLightControl(uint16_t lightRaw,
                            uint16_t threshold,
                            uint8_t autoMode,
                            uint8_t led0On,
                            uint8_t alarmActive);

protected:
    static const uint16_t LIGHT_SIZE = 8;
    static const uint16_t THRESHOLD_SIZE = 8;
    static const uint16_t MODE_SIZE = 8;
    static const uint16_t LED0_SIZE = 8;
    static const uint16_t ALARM_SIZE = 12;

    touchgfx::Unicode::UnicodeChar lightBuffer[LIGHT_SIZE];
    touchgfx::Unicode::UnicodeChar thresholdBuffer[THRESHOLD_SIZE];
    touchgfx::Unicode::UnicodeChar modeBuffer[MODE_SIZE];
    touchgfx::Unicode::UnicodeChar led0Buffer[LED0_SIZE];
    touchgfx::Unicode::UnicodeChar alarmBuffer[ALARM_SIZE];
};

#endif // LEDSCREENVIEW_HPP
