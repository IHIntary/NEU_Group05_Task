#ifndef PULSESCREENPRESENTER_HPP
#define PULSESCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class PulseScreenView;

class PulseScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    PulseScreenPresenter(PulseScreenView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    void startPulse();
    void endPulse();

    virtual void pulseDataUpdated(uint32_t ir, uint32_t red, uint16_t heartRate, float spo2, uint8_t progressPercent);

    virtual ~PulseScreenPresenter() {}

private:
    PulseScreenPresenter();

    PulseScreenView& view;

    uint32_t lastIr;
    uint32_t lastRed;
    uint16_t lastHeartRate;
    uint8_t lastProgressPercent;
    int16_t lastSpo2Tenths;
};

#endif // PULSESCREENPRESENTER_HPP
