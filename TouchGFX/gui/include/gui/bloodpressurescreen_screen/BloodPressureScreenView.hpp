#ifndef BLOODPRESSURESCREENVIEW_HPP
#define BLOODPRESSURESCREENVIEW_HPP

#include <gui_generated/bloodpressurescreen_screen/BloodPressureScreenViewBase.hpp>
#include <gui/bloodpressurescreen_screen/BloodPressureScreenPresenter.hpp>
#include <touchgfx/Unicode.hpp>
#include <stdint.h>

class BloodPressureScreenView : public BloodPressureScreenViewBase
{
public:
    BloodPressureScreenView();
    virtual ~BloodPressureScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void startBPClicked();
    virtual void resetBPClicked();

    virtual void unitToggleClicked();

    void updatePressure(float pressure);
    void updateSpeed(float speed);
    void updateState(const char* state);
    void updateHint(const char* hint);
    void updateResult(float sbp, float dbp, uint8_t hr, uint8_t valid);
    void clearBPGraph();
    void addBPGraphPoint(float pressure);
protected:
    static const uint16_t PRESSURE_SIZE = 24;
    static const uint16_t SPEED_SIZE = 24;
    static const uint16_t STATE_SIZE = 16;
    static const uint16_t HINT_SIZE = 32;
    static const uint16_t RESULT_SIZE = 24;

    int lastPressureValue;
    int lastSpeedTenths;
    char lastStateText[STATE_SIZE];

    uint8_t displayPa;
    float lastPressureMmHg;
    float lastSpeedMmHg;
    float lastSbpMmHg;
    float lastDbpMmHg;
    uint8_t lastHr;
    uint8_t lastResultValid;

    touchgfx::Unicode::UnicodeChar pressureBuffer[PRESSURE_SIZE];
    touchgfx::Unicode::UnicodeChar speedBuffer[SPEED_SIZE];
    touchgfx::Unicode::UnicodeChar stateBuffer[STATE_SIZE];
    touchgfx::Unicode::UnicodeChar hintBuffer[HINT_SIZE];
    touchgfx::Unicode::UnicodeChar sbpBuffer[RESULT_SIZE];
    touchgfx::Unicode::UnicodeChar dbpBuffer[RESULT_SIZE];
    touchgfx::Unicode::UnicodeChar hrBuffer[RESULT_SIZE];
};

#endif // BLOODPRESSURESCREENVIEW_HPP
