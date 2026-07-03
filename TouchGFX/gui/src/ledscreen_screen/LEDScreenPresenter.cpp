#include <gui/ledscreen_screen/LEDScreenView.hpp>
#include <gui/ledscreen_screen/LEDScreenPresenter.hpp>
#include <gui/model/Model.hpp>

LEDScreenPresenter::LEDScreenPresenter(LEDScreenView& v)
    : view(v)
{

}

void LEDScreenPresenter::activate()
{

}

void LEDScreenPresenter::deactivate()
{

}

void LEDScreenPresenter::lightControlUpdated(uint16_t lightRaw,
                                             uint16_t threshold,
                                             uint8_t autoMode,
                                             uint8_t led0On,
                                             uint8_t alarmActive)
{
    view.updateLightControl(lightRaw, threshold, autoMode, led0On, alarmActive);
}

void LEDScreenPresenter::lightModeClicked()
{
    model->toggleLightMode();
}

void LEDScreenPresenter::led0ToggleClicked()
{
    model->toggleLed0Manual();
}

void LEDScreenPresenter::alarmToggleClicked()
{
    model->toggleAlarmManual();
}
