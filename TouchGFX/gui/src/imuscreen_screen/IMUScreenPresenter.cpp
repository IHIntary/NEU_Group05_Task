#include <gui/imuscreen_screen/IMUScreenView.hpp>
#include <gui/imuscreen_screen/IMUScreenPresenter.hpp>
#include <gui/model/Model.hpp>

IMUScreenPresenter::IMUScreenPresenter(IMUScreenView& v)
    : view(v)
{

}

void IMUScreenPresenter::activate()
{
    model->setImuRunning(1U);
}

void IMUScreenPresenter::deactivate()
{
    model->setImuRunning(0U);
}

void IMUScreenPresenter::imuDataUpdated(const int16_t accMg[3],
                                        uint8_t ready,
                                        uint8_t dataValid,
                                        uint8_t fallDetected,
                                        uint8_t alarmActive)
{
    view.updateImuData(accMg, ready, dataValid, fallDetected, alarmActive);
}

void IMUScreenPresenter::imuAckClicked()
{
    model->clearImuAlarm();
}
