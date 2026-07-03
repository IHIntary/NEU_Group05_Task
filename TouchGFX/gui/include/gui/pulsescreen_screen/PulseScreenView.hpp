#ifndef PULSESCREENVIEW_HPP
#define PULSESCREENVIEW_HPP

#include <gui_generated/pulsescreen_screen/PulseScreenViewBase.hpp>
#include <gui/pulsescreen_screen/PulseScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>

class PulseScreenView : public PulseScreenViewBase
{
public:
    PulseScreenView();
    virtual ~PulseScreenView() {}

    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void startPulse();
    virtual void endPulse();

    void clearPulse();
    void showPulseProgress(uint8_t progressPercent);
    void showSpo2(int16_t spo2Tenths);
    void showHeartRate(uint16_t heartRate);
    void showIr(uint32_t ir);
    void showRed(uint32_t red);
		void addPulseGraphPoint(uint32_t heartRate);

protected:
    static const uint16_t SPO2TEXT_SIZE = 16;
    static const uint16_t HEARTRATETEXT_SIZE = 16;
    static const uint16_t IRTEXT_SIZE = 16;
    static const uint16_t REDTEXT_SIZE = 16;

    touchgfx::Unicode::UnicodeChar spo2TextBuffer[SPO2TEXT_SIZE];
    touchgfx::Unicode::UnicodeChar heartRateTextBuffer[HEARTRATETEXT_SIZE];
    touchgfx::Unicode::UnicodeChar irTextBuffer[IRTEXT_SIZE];
    touchgfx::Unicode::UnicodeChar redTextBuffer[REDTEXT_SIZE];
};

#endif
