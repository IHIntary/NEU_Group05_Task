#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <stdint.h>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    virtual void ecgDataUpdated(uint16_t raw, uint16_t filtered, uint8_t leadsOff, uint8_t running) {}
    virtual void pressureDataUpdated(uint16_t raw, uint32_t mmHgTenths, uint8_t running) {}
    virtual void pulseDataUpdated(uint32_t ir, uint32_t red, uint16_t heartRate, float spo2, uint8_t progressPercent) {}
    virtual void bpDataUpdated(float pressure, float speed, const char* state, const char* hint,
                               float sbp, float dbp, uint8_t hr, uint8_t valid) {}
    virtual void chipTempUpdated(float tempC, uint8_t valid) {}
    virtual void lightControlUpdated(uint16_t lightRaw,
                                     uint16_t threshold,
                                     uint8_t autoMode,
                                     uint8_t led0On,
                                     uint8_t alarmActive) {}

    void bind(Model* m)
    {
        model = m;
    }
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
