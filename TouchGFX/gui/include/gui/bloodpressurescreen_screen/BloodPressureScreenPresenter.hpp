#ifndef BLOODPRESSURESCREENPRESENTER_HPP
#define BLOODPRESSURESCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class BloodPressureScreenView;

class BloodPressureScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    BloodPressureScreenPresenter(BloodPressureScreenView& v);

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

    void startBPMeasure();
    void resetBPMeasure();
    virtual void bpDataUpdated(float pressure, float speed, const char* state, const char* hint,
                               float sbp, float dbp, uint8_t hr, uint8_t valid);

    virtual ~BloodPressureScreenPresenter() {}

private:
    BloodPressureScreenPresenter();

    BloodPressureScreenView& view;
    uint8_t graphRunning;
    uint8_t graphTickDivider;
    uint8_t uiTickDivider;
};

#endif // BLOODPRESSURESCREENPRESENTER_HPP
