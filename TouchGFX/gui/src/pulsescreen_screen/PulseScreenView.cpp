#include <gui/pulsescreen_screen/PulseScreenView.hpp>

PulseScreenView::PulseScreenView()
{
}

void PulseScreenView::setupScreen()
{
    PulseScreenViewBase::setupScreen();

    spo2Text.setWildcard(spo2TextBuffer);
    heartRateText.setWildcard(heartRateTextBuffer);
    irText.setWildcard(irTextBuffer);
    redText.setWildcard(redTextBuffer);

    spo2Text.setWidth(260);
    heartRateText.setWidth(260);
    irText.setWidth(340);
    redText.setWidth(340);

    clearPulse();
}

void PulseScreenView::tearDownScreen()
{
    PulseScreenViewBase::tearDownScreen();
}

void PulseScreenView::startPulse()
{
    presenter->startPulse();
}

void PulseScreenView::endPulse()
{
    presenter->endPulse();
}

void PulseScreenView::clearPulse()
{
    pulseProgress.setValue(0);
    touchgfx::Unicode::snprintf(spo2TextBuffer, SPO2TEXT_SIZE, "--.-");
    touchgfx::Unicode::snprintf(heartRateTextBuffer, HEARTRATETEXT_SIZE, "--");
    touchgfx::Unicode::snprintf(irTextBuffer, IRTEXT_SIZE, "0");
    touchgfx::Unicode::snprintf(redTextBuffer, REDTEXT_SIZE, "0");

    pulseProgress.invalidate();
    spo2Text.invalidate();
    heartRateText.invalidate();
    irText.invalidate();
    redText.invalidate();
}

void PulseScreenView::showPulseProgress(uint8_t progressPercent)
{
    pulseProgress.setValue(progressPercent);
    pulseProgress.invalidate();
}

void PulseScreenView::showSpo2(int16_t spo2Tenths)
{
    touchgfx::Unicode::snprintf(
        spo2TextBuffer,
        SPO2TEXT_SIZE,
        "%d.%d",
        spo2Tenths / 10,
        spo2Tenths % 10
    );
    spo2Text.invalidate();
}

void PulseScreenView::showHeartRate(uint16_t heartRate)
{
    touchgfx::Unicode::snprintf(
        heartRateTextBuffer,
        HEARTRATETEXT_SIZE,
        "%u",
        (unsigned int)heartRate
    );
    heartRateText.invalidate();
}

void PulseScreenView::showIr(uint32_t ir)
{
    touchgfx::Unicode::snprintf(
        irTextBuffer,
        IRTEXT_SIZE,
        "%u",
        (unsigned int)ir
    );
    irText.invalidate();
}

void PulseScreenView::showRed(uint32_t red)
{
    touchgfx::Unicode::snprintf(
        redTextBuffer,
        REDTEXT_SIZE,
        "%u",
        (unsigned int)red
    );
    redText.invalidate();
}
