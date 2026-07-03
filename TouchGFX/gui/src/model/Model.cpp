#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C"
{
#include "app_sensor_service.h"
#include "app_alarm_service.h"
#include "app_buzzer_service.h"
#include "app_light_service.h"
#include "bp_service.h"
#include "main.h"
}

Model::Model()
    : modelListener(0),
      lastKey2Pressed(0),
      key2StableTicks(0),
      ecgNotificationsEnabled(0U),
      pressureNotificationsEnabled(0U),
      pulseNotificationsEnabled(0U),
      bpNotificationsEnabled(0U)
{
    BP_Service_Init();
    AppAlarm_Init();
    AppLightService_Init();
}

void Model::tick()
{
    handleKey2();
    AppLightService_Tick();
    BP_Service_Tick();

    if (modelListener != 0)
    {
        SensorData_t data;
        AppLightStatus_t light;

        SensorService_GetData(&data);
        AppLightService_GetStatus(&light);

        modelListener->chipTempUpdated(data.chipTempC, data.chipTempValid);
        modelListener->lightControlUpdated(light.lightRaw,
                                           light.threshold,
                                           light.autoMode,
                                           light.led0On,
                                           light.alarmActive);

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
    AppLightService_ToggleLed0Manual();
}

void Model::toggleLed1()
{
    AppLightService_ToggleAlarmManual();
}

void Model::toggleLightMode()
{
    AppLightService_ToggleMode();
}

void Model::toggleLed0Manual()
{
    AppLightService_ToggleLed0Manual();
}

void Model::toggleAlarmManual()
{
    AppLightService_ToggleAlarmManual();
}

void Model::toggleAlarmAuto()
{
    AppLightService_ToggleAlarmAuto();
}

void Model::adjustLightThreshold(int16_t delta)
{
    AppLightService_AdjustThreshold(delta);
}

uint8_t Model::isLed0On() const
{
    AppLightStatus_t light;
    AppLightService_GetStatus(&light);
    return light.led0On;
}

void Model::handleKey2()
{
    uint8_t key2Pressed = (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) ? 1U : 0U;

    if (key2Pressed != lastKey2Pressed)
    {
        key2StableTicks++;
        if (key2StableTicks >= 2U)
        {
            lastKey2Pressed = key2Pressed;
            key2StableTicks = 0U;

            if (key2Pressed != 0U)
            {
                AppLightService_HandleKey2Pressed();
            }
        }
    }
    else
    {
        key2StableTicks = 0U;
    }
}

