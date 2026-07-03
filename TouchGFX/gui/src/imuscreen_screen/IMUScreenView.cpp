#include <gui/imuscreen_screen/IMUScreenView.hpp>

IMUScreenView::IMUScreenView()
{

}

void IMUScreenView::setupScreen()
{
    IMUScreenViewBase::setupScreen();
    txtImuStatus.setWildcard(statusBuffer);
    txtImuFall.setWildcard(fallBuffer);
    txtImuAccX.setWildcard(accXBuffer);
    txtImuAccY.setWildcard(accYBuffer);
    txtImuAccZ.setWildcard(accZBuffer);

    int16_t zeroAcc[3] = {0, 0, 0};
    updateImuData(zeroAcc, 0U, 0U, 0U, 0U);
}

void IMUScreenView::tearDownScreen()
{
    IMUScreenViewBase::tearDownScreen();
}

void IMUScreenView::imuAckClicked()
{
    presenter->imuAckClicked();
}

void IMUScreenView::updateImuData(const int16_t accMg[3],
                                  uint8_t ready,
                                  uint8_t dataValid,
                                  uint8_t fallDetected,
                                  uint8_t alarmActive)
{
    if (ready == 0U)
    {
        touchgfx::Unicode::snprintf(statusBuffer, STATUS_SIZE, "NO CHIP");
    }
    else if (dataValid == 0U)
    {
        touchgfx::Unicode::snprintf(statusBuffer, STATUS_SIZE, "WAIT");
    }
    else
    {
        touchgfx::Unicode::snprintf(statusBuffer, STATUS_SIZE, "READY");
    }

    touchgfx::Unicode::snprintf(fallBuffer, FALL_SIZE, (alarmActive != 0U || fallDetected != 0U) ? "ALARM" : "NORMAL");
    touchgfx::Unicode::snprintf(accXBuffer, VALUE_SIZE, "%d", (int)accMg[0]);
    touchgfx::Unicode::snprintf(accYBuffer, VALUE_SIZE, "%d", (int)accMg[1]);
    touchgfx::Unicode::snprintf(accZBuffer, VALUE_SIZE, "%d", (int)accMg[2]);

    txtImuStatus.invalidate();
    txtImuFall.invalidate();
    txtImuAccX.invalidate();
    txtImuAccY.invalidate();
    txtImuAccZ.invalidate();
}
