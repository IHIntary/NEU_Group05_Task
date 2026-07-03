#include <gui/common/FrontendApplication.hpp>

static const uint8_t REMOTE_KEY_BACK = 0x4AU;
static const uint8_t REMOTE_KEY_1 = 0x16U;
static const uint8_t REMOTE_KEY_2 = 0x19U;
static const uint8_t REMOTE_KEY_3 = 0x0DU;
static const uint8_t REMOTE_KEY_4 = 0x0CU;
static const uint8_t REMOTE_KEY_5 = 0x18U;
static const uint8_t REMOTE_KEY_6 = 0x5EU;
static const uint8_t REMOTE_KEY_LOG = 0x45U;

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      currentScreen(SCREEN_LOGIN)
{

}

void FrontendApplication::gotoLOGINScreenNoTransition()
{
    currentScreen = SCREEN_LOGIN;
    FrontendApplicationBase::gotoLOGINScreenNoTransition();
}

void FrontendApplication::gotoHomeScreenScreenNoTransition()
{
    currentScreen = SCREEN_HOME;
    FrontendApplicationBase::gotoHomeScreenScreenNoTransition();
}

void FrontendApplication::gotoHomeScreenScreenSlideTransitionWest()
{
    currentScreen = SCREEN_HOME;
    FrontendApplicationBase::gotoHomeScreenScreenSlideTransitionWest();
}

void FrontendApplication::gotoECGScreenScreenSlideTransitionEast()
{
    currentScreen = SCREEN_ECG;
    FrontendApplicationBase::gotoECGScreenScreenSlideTransitionEast();
}

void FrontendApplication::gotoPulseScreenScreenSlideTransitionEast()
{
    currentScreen = SCREEN_PULSE;
    FrontendApplicationBase::gotoPulseScreenScreenSlideTransitionEast();
}

void FrontendApplication::gotoBloodPressureScreenScreenSlideTransitionEast()
{
    currentScreen = SCREEN_BLOOD_PRESSURE;
    FrontendApplicationBase::gotoBloodPressureScreenScreenSlideTransitionEast();
}

void FrontendApplication::gotoLEDScreenScreenSlideTransitionEast()
{
    currentScreen = SCREEN_LED;
    FrontendApplicationBase::gotoLEDScreenScreenSlideTransitionEast();
}

void FrontendApplication::gotoClockScreenScreenSlideTransitionEast()
{
    currentScreen = SCREEN_CLOCK;
    FrontendApplicationBase::gotoClockScreenScreenSlideTransitionEast();
}

void FrontendApplication::gotoIMUScreenScreenSlideTransitionEast()
{
    currentScreen = SCREEN_IMU;
    FrontendApplicationBase::gotoIMUScreenScreenSlideTransitionEast();
}

void FrontendApplication::setCurrentScreenHome()
{
    currentScreen = SCREEN_HOME;
}

void FrontendApplication::handleRemoteKey(uint8_t key)
{
    if (key == REMOTE_KEY_LOG)
    {
        if (currentScreen == SCREEN_LOGIN)
        {
            gotoHomeScreenScreenNoTransition();
        }
        else if (currentScreen == SCREEN_HOME)
        {
            gotoLOGINScreenNoTransition();
        }
        return;
    }

    if (key == REMOTE_KEY_BACK)
    {
        if ((currentScreen != SCREEN_HOME) && (currentScreen != SCREEN_LOGIN))
        {
            gotoHomeScreenScreenSlideTransitionWest();
        }
        return;
    }

    if (currentScreen != SCREEN_HOME)
    {
        return;
    }

    switch (key)
    {
    case REMOTE_KEY_1:
        gotoECGScreenScreenSlideTransitionEast();
        break;
    case REMOTE_KEY_2:
        gotoPulseScreenScreenSlideTransitionEast();
        break;
    case REMOTE_KEY_3:
        gotoBloodPressureScreenScreenSlideTransitionEast();
        break;
    case REMOTE_KEY_4:
        gotoLEDScreenScreenSlideTransitionEast();
        break;
    case REMOTE_KEY_5:
        gotoClockScreenScreenSlideTransitionEast();
        break;
    case REMOTE_KEY_6:
        gotoIMUScreenScreenSlideTransitionEast();
        break;
    default:
        break;
    }
}
