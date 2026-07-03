#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <stdint.h>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    void gotoLOGINScreenNoTransition();
    void gotoHomeScreenScreenNoTransition();
    void gotoHomeScreenScreenSlideTransitionWest();
    void gotoECGScreenScreenSlideTransitionEast();
    void gotoPulseScreenScreenSlideTransitionEast();
    void gotoBloodPressureScreenScreenSlideTransitionEast();
    void gotoLEDScreenScreenSlideTransitionEast();
    void gotoClockScreenScreenSlideTransitionEast();
    void gotoIMUScreenScreenSlideTransitionEast();
    void handleRemoteKey(uint8_t key);
    void setCurrentScreenHome();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }
private:
    enum ScreenId
    {
        SCREEN_LOGIN,
        SCREEN_HOME,
        SCREEN_ECG,
        SCREEN_PULSE,
        SCREEN_BLOOD_PRESSURE,
        SCREEN_LED,
        SCREEN_CLOCK,
        SCREEN_IMU
    };

    ScreenId currentScreen;
};

#endif // FRONTENDAPPLICATION_HPP
