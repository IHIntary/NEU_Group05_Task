#include <gui/ecgscreen_screen/ECGScreenView.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

ECGScreenView::ECGScreenView()
{
}

void ECGScreenView::setupScreen()
{
    ECGScreenViewBase::setupScreen();
    ecgRawText.setWildcard(ecgRawTextBuffer);
    ecgRawText.setWidth(180);
    ecgRawText.setHeight(40);
    clearEcg();
}

void ECGScreenView::tearDownScreen()
{
    ECGScreenViewBase::tearDownScreen();
}

void ECGScreenView::function8()
{
    startECG();
}

void ECGScreenView::startECG()
{
    presenter->startECG();
}

void ECGScreenView::pauseECG()
{
    presenter->pauseECG();
}

void ECGScreenView::resetECG()
{
    presenter->resetECG();
}

void ECGScreenView::toggleLED1()
{
    presenter->toggleLED1();
}

void ECGScreenView::toggleLED0()
{
    presenter->toggleLED0();
}

void ECGScreenView::prepareEcgStart()
{
    leadsOffText.setVisible(false);
    leadsOffText.invalidate();
}

void ECGScreenView::clearEcg()
{
    ecgGraph.clear();
    ecgGraph.invalidate();

    touchgfx::Unicode::snprintf(ecgRawTextBuffer, ECGRAWTEXT_SIZE, "--");
    ecgRawText.invalidate();

    leadsOffText.setVisible(false);
    leadsOffText.invalidate();
}

void ECGScreenView::showEcgRaw(uint16_t raw)
{
    touchgfx::Unicode::snprintf(ecgRawTextBuffer, ECGRAWTEXT_SIZE, "%u", (unsigned int)raw);
    ecgRawText.invalidate();
}

void ECGScreenView::showLeadsOff(uint8_t leadsOff)
{
    leadsOffText.setVisible(leadsOff != 0U);
    leadsOffText.invalidate();
}

void ECGScreenView::addEcgPoint(uint16_t filtered)
{
    ecgGraph.addDataPoint((int)filtered);
}

void ECGScreenView::showLed0State(uint8_t on)
{
    if (on != 0U)
    {
        led0StateBox.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    }
    else
    {
        led0StateBox.setColor(touchgfx::Color::getColorFromRGB(150, 150, 150));
    }

    led0StateBox.invalidate();
}
