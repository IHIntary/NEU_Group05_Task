#ifndef IMUSCREENVIEW_HPP
#define IMUSCREENVIEW_HPP

#include <gui_generated/imuscreen_screen/IMUScreenViewBase.hpp>
#include <gui/imuscreen_screen/IMUScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>
#include <stdint.h>

class IMUScreenView : public IMUScreenViewBase
{
public:
    IMUScreenView();
    virtual ~IMUScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void imuAckClicked();

    void updateImuData(const int16_t accMg[3],
                       uint8_t ready,
                       uint8_t dataValid,
                       uint8_t fallDetected,
                       uint8_t alarmActive);
protected:
    static const uint16_t STATUS_SIZE = 12;
    static const uint16_t FALL_SIZE = 12;
    static const uint16_t VALUE_SIZE = 16;

    touchgfx::Unicode::UnicodeChar statusBuffer[STATUS_SIZE];
    touchgfx::Unicode::UnicodeChar fallBuffer[FALL_SIZE];
    touchgfx::Unicode::UnicodeChar accXBuffer[VALUE_SIZE];
    touchgfx::Unicode::UnicodeChar accYBuffer[VALUE_SIZE];
    touchgfx::Unicode::UnicodeChar accZBuffer[VALUE_SIZE];
};

#endif // IMUSCREENVIEW_HPP
