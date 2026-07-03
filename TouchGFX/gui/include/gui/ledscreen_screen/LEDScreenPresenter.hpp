#ifndef LEDSCREENPRESENTER_HPP
#define LEDSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class LEDScreenView;

class LEDScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    LEDScreenPresenter(LEDScreenView& v);

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

    virtual void lightControlUpdated(uint16_t lightRaw,
                                     uint16_t threshold,
                                     uint8_t autoMode,
                                     uint8_t led0On,
                                     uint8_t alarmActive);

    void lightModeClicked();
    void led0ToggleClicked();
    void alarmToggleClicked();

    virtual ~LEDScreenPresenter() {}

private:
    LEDScreenPresenter();

    LEDScreenView& view;
};

#endif // LEDSCREENPRESENTER_HPP
