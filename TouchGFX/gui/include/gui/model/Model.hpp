#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

extern "C"
{
#include "app_rtc_service.h"
}

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
    void setImuRunning(uint8_t running);
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
    void clearImuAlarm();
    uint8_t isLed0On() const;
    void getRtcDateTime(AppRtcDateTime_t *out) const;
    void getRtcStatus(AppRtcStatus_t *out) const;
    uint8_t setRtcDateTime(const AppRtcDateTime_t *dateTime);
    uint8_t sendRtcDateTimeHex(const AppRtcDateTime_t *dateTime);
protected:
    ModelListener* modelListener;

private:
    void handleKey2();
    void handleRemote();

    uint8_t lastKey2Pressed;
    uint8_t key2StableTicks;
    uint8_t lastRemoteKey;
    uint8_t ecgNotificationsEnabled;
    uint8_t pressureNotificationsEnabled;
    uint8_t pulseNotificationsEnabled;
    uint8_t bpNotificationsEnabled;
};

#endif // MODEL_HPP
