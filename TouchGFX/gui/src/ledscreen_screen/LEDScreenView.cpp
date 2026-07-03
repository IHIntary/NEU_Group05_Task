#include <gui/ledscreen_screen/LEDScreenView.hpp>

LEDScreenView::LEDScreenView()
{

}

void LEDScreenView::setupScreen()
{
    LEDScreenViewBase::setupScreen();
    txtLightRaw.setWildcard(lightBuffer);
    txtLightThreshold.setWildcard(thresholdBuffer);
    txtLightMode.setWildcard(modeBuffer);
    txtLed0State.setWildcard(led0Buffer);
    txtAlarmState.setWildcard(alarmBuffer);

    updateLightControl(0U, 150U, 1U, 0U, 0U);
}

void LEDScreenView::tearDownScreen()
{
    LEDScreenViewBase::tearDownScreen();
}

void LEDScreenView::lightModeClicked()
{
    presenter->lightModeClicked();
}

void LEDScreenView::led0ToggleClicked()
{
    presenter->led0ToggleClicked();
}

void LEDScreenView::alarmToggleClicked()
{
    presenter->alarmToggleClicked();
}

void LEDScreenView::updateLightControl(uint16_t lightRaw,
                                       uint16_t threshold,
                                       uint8_t autoMode,
                                       uint8_t led0On,
                                       uint8_t alarmActive)
{
    touchgfx::Unicode::snprintf(lightBuffer, LIGHT_SIZE, "%04u", (unsigned int)lightRaw);
    touchgfx::Unicode::snprintf(thresholdBuffer, THRESHOLD_SIZE, "%04u", (unsigned int)threshold);
    touchgfx::Unicode::snprintf(modeBuffer, MODE_SIZE, (autoMode != 0U) ? "AUTO" : "MANUAL");
    touchgfx::Unicode::snprintf(led0Buffer, LED0_SIZE, (led0On != 0U) ? "ON" : "OFF");
    touchgfx::Unicode::snprintf(alarmBuffer, ALARM_SIZE, (alarmActive != 0U) ? "ACTIVE" : "OFF");

    txtLightRaw.invalidate();
    txtLightThreshold.invalidate();
    txtLightMode.invalidate();
    txtLed0State.invalidate();
    txtAlarmState.invalidate();
}
