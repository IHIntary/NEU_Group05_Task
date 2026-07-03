#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();
    void setEcgRunning(uint8_t running);
    void setPressureRunning(uint8_t running);
    void setPulseRunning(uint8_t running);
    void startBPMeasure();
    void resetBPMeasure();
    void toggleLed0();
    void toggleLed1();
    void toggleLightMode();
    void toggleLed0Manual();
    void toggleAlarmManual();
    void toggleAlarmAuto();
    void adjustLightThreshold(int16_t delta);
    void beep(uint16_t durationMs);
    uint8_t isLed0On() const;
protected:
    ModelListener* modelListener;

private:
    void handleKey2();

    uint8_t lastKey2Pressed;
    uint8_t key2StableTicks;
    uint8_t ecgNotificationsEnabled;
    uint8_t pressureNotificationsEnabled;
    uint8_t pulseNotificationsEnabled;
    uint8_t bpNotificationsEnabled;
};

#endif // MODEL_HPP
