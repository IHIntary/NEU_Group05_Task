#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C"
{
#include "app_sensor_service.h"
#include "app_alarm_service.h"
#include "app_buzzer_service.h"
#include "bp_service.h"
#include "main.h"
}

#define LED_ON_STATE  GPIO_PIN_RESET
#define LED_OFF_STATE GPIO_PIN_SET

static void setLed0(uint8_t on)
{
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, on ? LED_ON_STATE : LED_OFF_STATE);
}

static uint8_t isLed1On()
{
    return (HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin) == LED_ON_STATE) ? 1U : 0U;
}

static void setLed1(uint8_t on)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, on ? LED_ON_STATE : LED_OFF_STATE);
}

Model::Model()
    : modelListener(0),
      lastKey0Pressed(0),
      key0StableTicks(0),
      lastNotifiedLed0On(0xFFU),
      ecgNotificationsEnabled(0U),
      pressureNotificationsEnabled(0U),
      pulseNotificationsEnabled(0U),
      bpNotificationsEnabled(0U)
{
    BP_Service_Init();
    AppAlarm_Init();
}

void Model::tick()
{
    handleKey0();
    notifyLed0IfChanged();
    BP_Service_Tick();

    if (modelListener != 0)
    {
        SensorData_t data;
        SensorService_GetData(&data);
        modelListener->chipTempUpdated(data.chipTempC, data.chipTempValid);

        if ((ecgNotificationsEnabled != 0U) ||
            (pressureNotificationsEnabled != 0U) ||
            (pulseNotificationsEnabled != 0U) ||
            (bpNotificationsEnabled != 0U))
        {
            AppAlarmInput_t alarmInput;
            alarmInput.sensor = data;
            alarmInput.bpState = BP_Service_GetState();
            alarmInput.bpPressure = BP_Service_GetPressure();
            alarmInput.bpDeflateSpeed = BP_Service_GetDeflateSpeed();
            alarmInput.bpSbp = BP_Service_GetSBP();
            alarmInput.bpDbp = BP_Service_GetDBP();
            alarmInput.bpHr = BP_Service_GetHR();
            alarmInput.bpResultValid = BP_Service_IsResultValid();
            AppAlarm_Update(&alarmInput);

            if (ecgNotificationsEnabled != 0U)
            {
                modelListener->ecgDataUpdated(data.ecgRaw, data.ecgFiltered, data.ecgLeadsOff, data.ecgRunning);
            }

            if (pressureNotificationsEnabled != 0U)
            {
                modelListener->pressureDataUpdated(data.pressureRaw, data.pressureMmHgTenths, data.pressureRunning);
            }

            if (pulseNotificationsEnabled != 0U)
            {
                modelListener->pulseDataUpdated(data.ppgIr, data.ppgRed, data.heartRate, data.spo2, data.pulseProgressPercent);
            }

            if (bpNotificationsEnabled != 0U)
            {
                modelListener->bpDataUpdated(BP_Service_GetPressure(),
                                             BP_Service_GetDeflateSpeed(),
                                             BP_Service_GetStateText(),
                                             BP_Service_GetHintText(),
                                             alarmInput.bpSbp,
                                             alarmInput.bpDbp,
                                             alarmInput.bpHr,
                                             alarmInput.bpResultValid);
            }
        }
    }
}

void Model::beep(uint16_t durationMs)
{
    AppBuzzer_Beep(durationMs);
}

void Model::setEcgRunning(uint8_t running)
{
    if (running != 0U)
    {
        ecgNotificationsEnabled = 1U;
    }

    SensorService_SetEcgRunning(running);
}

void Model::setPressureRunning(uint8_t running)
{
    if (running != 0U)
    {
        pressureNotificationsEnabled = 1U;
    }

    SensorService_SetPressureRunning(running);
}

void Model::setPulseRunning(uint8_t running)
{
    if (running != 0U)
    {
        pulseNotificationsEnabled = 1U;
    }

    SensorService_SetPulseRunning(running);
}

void Model::startBPMeasure()
{
    bpNotificationsEnabled = 1U;
    BP_Service_Start();
}

void Model::resetBPMeasure()
{
    bpNotificationsEnabled = 1U;
    BP_Service_Reset();
}

void Model::toggleLed0()
{
    setLed0(isLed0On() == 0U);
    notifyLed0IfChanged();
}

void Model::toggleLed1()
{
    setLed1(isLed1On() == 0U);
}

uint8_t Model::isLed0On() const
{
    return (HAL_GPIO_ReadPin(LED0_GPIO_Port, LED0_Pin) == LED_ON_STATE) ? 1U : 0U;
}

void Model::handleKey0()
{
    uint8_t key0Pressed = (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) ? 1U : 0U;

    if (key0Pressed != lastKey0Pressed)
    {
        key0StableTicks++;
        if (key0StableTicks >= 2U)
        {
            lastKey0Pressed = key0Pressed;
            key0StableTicks = 0U;

            if (key0Pressed != 0U)
            {
                toggleLed0();
            }
        }
    }
    else
    {
        key0StableTicks = 0U;
    }
}

void Model::notifyLed0IfChanged()
{
    uint8_t on = isLed0On();

    if (on == lastNotifiedLed0On)
    {
        return;
    }

    lastNotifiedLed0On = on;
    if (modelListener != 0)
    {
        modelListener->led0StateUpdated(on);
    }
}
