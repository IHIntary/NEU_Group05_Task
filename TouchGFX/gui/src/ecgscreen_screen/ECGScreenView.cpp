#include <gui/ecgscreen_screen/ECGScreenView.hpp>
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
