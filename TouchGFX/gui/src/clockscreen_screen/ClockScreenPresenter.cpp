#include <gui/clockscreen_screen/ClockScreenPresenter.hpp>
#include <gui/clockscreen_screen/ClockScreenView.hpp>

ClockScreenPresenter::ClockScreenPresenter(ClockScreenView& v)
    : view(v)
{
}

void ClockScreenPresenter::activate()
{
    AppRtcDateTime_t now;
    AppRtcStatus_t status;
    status.ready = 0U;
    status.usingLse = 0U;
    status.backupValid = 0U;
    model->getRtcDateTime(&now);
    model->getRtcStatus(&status);
    view.updateRtc(now, status);
}

void ClockScreenPresenter::deactivate()
{
}

void ClockScreenPresenter::rtcTimeUpdated(const AppRtcDateTime_t& dateTime, const AppRtcStatus_t& status)
{
    view.updateRtc(dateTime, status);
}

uint8_t ClockScreenPresenter::setRtcDateTime(const AppRtcDateTime_t& dateTime)
{
    return model->setRtcDateTime(&dateTime);
}

uint8_t ClockScreenPresenter::sendRtcDateTimeHex(const AppRtcDateTime_t& dateTime)
{
    return model->sendRtcDateTimeHex(&dateTime);
}

void ClockScreenPresenter::getRtcDateTime(AppRtcDateTime_t& out) const
{
    model->getRtcDateTime(&out);
}
