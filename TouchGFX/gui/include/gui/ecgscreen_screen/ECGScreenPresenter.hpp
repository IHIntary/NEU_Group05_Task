#ifndef ECGSCREENPRESENTER_HPP
#define ECGSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ECGScreenView;

class ECGScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ECGScreenPresenter(ECGScreenView& v);

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

    void startECG();
    void pauseECG();
    void resetECG();

    virtual void ecgDataUpdated(uint16_t raw, uint16_t filtered, uint8_t leadsOff, uint8_t running);

    virtual ~ECGScreenPresenter() {}

private:
    ECGScreenPresenter();

    ECGScreenView& view;

    uint16_t tickCounter;
    uint16_t rawTextTickCounter;
    uint16_t lastEcgRaw;
    uint8_t lastLeadsOff;
    uint8_t uiStarted;
};

#endif // ECGSCREENPRESENTER_HPP
