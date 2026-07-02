#include <gui/ecgscreen_screen/ECGScreenPresenter.hpp>
#include <gui/ecgscreen_screen/ECGScreenView.hpp>
#include <gui/model/Model.hpp>

ECGScreenPresenter::ECGScreenPresenter(ECGScreenView& v)
    : view(v),
      tickCounter(0),
      rawTextTickCounter(0),
      lastEcgRaw(0xFFFFU),
      lastLeadsOff(0xFFU),
      uiStarted(0)
{
}

void ECGScreenPresenter::activate()
{
    tickCounter = 0;
    rawTextTickCounter = 0;
    lastEcgRaw = 0xFFFFU;
    lastLeadsOff = 0xFFU;
    uiStarted = 0U;
    view.showLed0State(model->isLed0On());
}

void ECGScreenPresenter::deactivate()
{
    model->setEcgRunning(0U);
}

void ECGScreenPresenter::startECG()
{
    uiStarted = 1U;
    tickCounter = 0;
    rawTextTickCounter = 10U;
    lastEcgRaw = 0xFFFFU;
    lastLeadsOff = 0xFFU;

    model->setEcgRunning(1U);
    model->beep(60U);
    view.prepareEcgStart();
}

void ECGScreenPresenter::pauseECG()
{
    model->setEcgRunning(0U);
    model->beep(60U);
}

void ECGScreenPresenter::resetECG()
{
    model->setEcgRunning(0U);
    model->beep(60U);

    uiStarted = 0U;
    tickCounter = 0;
    rawTextTickCounter = 0;
    lastEcgRaw = 0xFFFFU;
    lastLeadsOff = 0xFFU;

    view.clearEcg();
}

void ECGScreenPresenter::toggleLED0()
{
    model->toggleLed0();
    view.showLed0State(model->isLed0On());
}

void ECGScreenPresenter::toggleLED1()
{
    model->toggleLed1();
}

void ECGScreenPresenter::ecgDataUpdated(uint16_t raw, uint16_t filtered, uint8_t leadsOff, uint8_t running)
{
    if (uiStarted == 0U)
    {
        return;
    }

    tickCounter++;
    rawTextTickCounter++;

    if ((lastEcgRaw != raw) && ((rawTextTickCounter >= 10U) || (running == 0U)))
    {
        rawTextTickCounter = 0;
        lastEcgRaw = raw;
        view.showEcgRaw(raw);
    }

    if (lastLeadsOff != leadsOff)
    {
        lastLeadsOff = leadsOff;
        view.showLeadsOff(leadsOff);
    }

    if ((leadsOff == 0U) && (running != 0U) && ((tickCounter % 3U) == 0U))
    {
        view.addEcgPoint(filtered);
    }
}

void ECGScreenPresenter::led0StateUpdated(uint8_t on)
{
    view.showLed0State(on);
}
