#include <gui/pulsescreen_screen/PulseScreenPresenter.hpp>
#include <gui/pulsescreen_screen/PulseScreenView.hpp>
#include <gui/model/Model.hpp>

PulseScreenPresenter::PulseScreenPresenter(PulseScreenView& v)
    : view(v),
      lastIr(0xFFFFFFFFUL),
      lastRed(0xFFFFFFFFUL),
      lastHeartRate(0xFFFFU),
      lastProgressPercent(0xFFU),
      lastSpo2Tenths(-1),
      graphRunning(0U),
      graphTickDivider(0U)
{
}

void PulseScreenPresenter::activate()
{
    lastIr = 0xFFFFFFFFUL;
    lastRed = 0xFFFFFFFFUL;
    lastHeartRate = 0xFFFFU;
    lastProgressPercent = 0xFFU;
    lastSpo2Tenths = -1;
    graphRunning = 0U;
    graphTickDivider = 0U;
    view.clearPulse();
}

void PulseScreenPresenter::deactivate()
{
    graphRunning = 0U;
    graphTickDivider = 0U;
    model->setPulseRunning(0U);
}

void PulseScreenPresenter::startPulse()
{
    lastIr = 0xFFFFFFFFUL;
    lastRed = 0xFFFFFFFFUL;
    lastHeartRate = 0xFFFFU;
    lastProgressPercent = 0xFFU;
    lastSpo2Tenths = -1;
    graphRunning = 1U;
    graphTickDivider = 0U;

    view.clearPulse();
    model->setPulseRunning(1U);
    model->beep(60U);
}

void PulseScreenPresenter::endPulse()
{
    graphRunning = 0U;
    graphTickDivider = 0U;
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
    }

    if (graphRunning != 0U)
    {
        graphTickDivider++;
        if (graphTickDivider >= 8U)
        {
            graphTickDivider = 0U;
            view.addPulseGraphPoints(heartRate, spo2Tenths);
        }
    }

    if (progressPercent >= 100U)
    {
        graphRunning = 0U;
        graphTickDivider = 0U;
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
