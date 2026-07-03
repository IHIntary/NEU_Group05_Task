#ifndef ECGSCREENVIEW_HPP
#define ECGSCREENVIEW_HPP

#include <gui_generated/ecgscreen_screen/ECGScreenViewBase.hpp>
#include <gui/ecgscreen_screen/ECGScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>

class ECGScreenView : public ECGScreenViewBase
{
public:
    ECGScreenView();
    virtual ~ECGScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void function8();
    virtual void startECG();
    virtual void pauseECG();
    virtual void resetECG();

    void prepareEcgStart();
    void clearEcg();
    void showEcgRaw(uint16_t raw);
    void showLeadsOff(uint8_t leadsOff);
    void addEcgPoint(uint16_t filtered);

protected:
    static const uint16_t ECGRAWTEXT_SIZE = 16;

    touchgfx::Unicode::UnicodeChar ecgRawTextBuffer[ECGRAWTEXT_SIZE];
};

#endif
