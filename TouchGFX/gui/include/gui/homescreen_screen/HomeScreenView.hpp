#ifndef HOMESCREENVIEW_HPP
#define HOMESCREENVIEW_HPP

#include <gui_generated/homescreen_screen/HomeScreenViewBase.hpp>
#include <gui/homescreen_screen/HomeScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>
#include <stdint.h>

class HomeScreenView : public HomeScreenViewBase
{
public:
    HomeScreenView();
    virtual ~HomeScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void updateChipTemp(float tempC, uint8_t valid);
protected:
    static const uint16_t CHIP_TEMP_SIZE = 16;

    touchgfx::Unicode::UnicodeChar chipTempBuffer[CHIP_TEMP_SIZE];
};

#endif // HOMESCREENVIEW_HPP
