#ifndef CLOCKSCREENPRESENTER_HPP
#define CLOCKSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class ClockScreenView;

class ClockScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ClockScreenPresenter(ClockScreenView& v);

    virtual void activate();
    virtual void deactivate();
    virtual void rtcTimeUpdated(const AppRtcDateTime_t& dateTime, const AppRtcStatus_t& status);

    uint8_t setRtcDateTime(const AppRtcDateTime_t& dateTime);
    uint8_t sendRtcDateTimeHex(const AppRtcDateTime_t& dateTime);
    void getRtcDateTime(AppRtcDateTime_t& out) const;

    virtual ~ClockScreenPresenter() {}

private:
    ClockScreenPresenter();

    ClockScreenView& view;
};

#endif // CLOCKSCREENPRESENTER_HPP
