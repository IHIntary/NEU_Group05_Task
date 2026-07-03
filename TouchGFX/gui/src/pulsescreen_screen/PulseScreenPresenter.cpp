#include <gui/pulsescreen_screen/PulseScreenPresenter.hpp>
#include <gui/pulsescreen_screen/PulseScreenView.hpp>
#include <gui/model/Model.hpp>

PulseScreenPresenter::PulseScreenPresenter(PulseScreenView& v)
    : view(v),
      lastIr(0xFFFFFFFFUL),
      lastRed(0xFFFFFFFFUL),
      lastHeartRate(0xFFFFU),
      lastProgressPercent(0xFFU),
      lastSpo2Tenths(-1)
{
}

void PulseScreenPresenter::activate()
{
    lastIr = 0xFFFFFFFFUL;
    lastRed = 0xFFFFFFFFUL;
    lastHeartRate = 0xFFFFU;
    lastProgressPercent = 0xFFU;
    lastSpo2Tenths = -1;
}

void PulseScreenPresenter::deactivate()
{
    model->setPulseRunning(0U);
}

void PulseScreenPresenter::startPulse()
{
    lastIr = 0xFFFFFFFFUL;
    lastRed = 0xFFFFFFFFUL;
    lastHeartRate = 0xFFFFU;
    lastProgressPercent = 0xFFU;
    lastSpo2Tenths = -1;

    view.clearPulse();
    model->setPulseRunning(1U);
    model->beep(60U);
}

void PulseScreenPresenter::endPulse()
{
    model->setPulseRunning(0U);
    model->beep(60U);
}

void PulseScreenPresenter::pulseDataUpdated(uint32_t ir, uint32_t red, uint16_t heartRate, float spo2, uint8_t progressPercent)
{
    if (lastProgressPercent != progressPercent)
    {
        lastProgressPercent = progressPercent;
        view.showPulseProgress(progressPercent);
    }

    int16_t spo2Tenths = (int16_t)(spo2 * 10.0f);
    if (lastSpo2Tenths != spo2Tenths)
    {
        lastSpo2Tenths = spo2Tenths;
        view.showSpo2(spo2Tenths);
    }

    if (lastHeartRate != heartRate)
    {
        lastHeartRate = heartRate;
        view.showHeartRate(heartRate);
				view.addPulseGraphPoint(heartRate);
    }

    if (lastIr != ir)
    {
        lastIr = ir;
        view.showIr(ir);
    }

    if (lastRed != red)
    {
        lastRed = red;
        view.showRed(red);
    }
}
