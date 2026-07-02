#include <gui/bloodpressurescreen_screen/BloodPressureScreenView.hpp>

static void copyAscii(touchgfx::Unicode::UnicodeChar* dst, uint16_t size, const char* src)
{
    uint16_t i = 0U;

    if ((dst == 0) || (size == 0U))
    {
        return;
    }

    if (src != 0)
    {
        while ((src[i] != '\0') && (i < (uint16_t)(size - 1U)))
        {
            dst[i] = (touchgfx::Unicode::UnicodeChar)src[i];
            i++;
        }
    }

    dst[i] = 0U;
}

BloodPressureScreenView::BloodPressureScreenView()
{

}

void BloodPressureScreenView::setupScreen()
{
    BloodPressureScreenViewBase::setupScreen();

    txtPressure.setWildcard(pressureBuffer);
    txtSpeed.setWildcard(speedBuffer);
    txtState.setWildcard(stateBuffer);
    txtHint.setWildcard(hintBuffer);
    txtSBP.setWildcard(sbpBuffer);
    txtDBP.setWildcard(dbpBuffer);
    txtHR.setWildcard(hrBuffer);
		
		lastPressureValue = -1;
		lastSpeedTenths = -1;
		lastStateText[0] = '\0';

    updatePressure(0.0f);
    updateSpeed(0.0f);
    updateState("IDLE");
    updateHint("Press START");
    updateResult(0.0f, 0.0f, 0U, 0U);
    clearBPGraph();
}

void BloodPressureScreenView::tearDownScreen()
{
    BloodPressureScreenViewBase::tearDownScreen();
}

void BloodPressureScreenView::startBPClicked()
{
    presenter->startBPMeasure();
}

void BloodPressureScreenView::resetBPClicked()
{
    presenter->resetBPMeasure();
}

void BloodPressureScreenView::updatePressure(float pressure)
{
    int value = (int)(pressure + 0.5f);
    if (value < 0)
    {
        value = 0;
    }

    if (value == lastPressureValue)
    {
        return;
    }
    lastPressureValue = value;

    touchgfx::Unicode::snprintf(pressureBuffer, PRESSURE_SIZE, "%03d", value);
    txtPressure.invalidate();
}

void BloodPressureScreenView::updateSpeed(float speed)
{
    int tenths;

    if (speed < 0.0f)
    {
        speed = 0.0f;
    }

    tenths = (int)((speed * 10.0f) + 0.5f);
    if (tenths == lastSpeedTenths)
    {
        return;
    }
    lastSpeedTenths = tenths;

    touchgfx::Unicode::snprintf(speedBuffer, SPEED_SIZE, "%d.%d", tenths / 10, tenths % 10);
    txtSpeed.invalidate();
}

void BloodPressureScreenView::updateState(const char* state)
{
    uint16_t i = 0U;
    uint8_t same = 1U;

    if (state == 0)
    {
        state = "";
    }

    while ((i < (uint16_t)(STATE_SIZE - 1U)) &&
           ((state[i] != '\0') || (lastStateText[i] != '\0')))
    {
        if (state[i] != lastStateText[i])
        {
            same = 0U;
            break;
        }
        i++;
    }

    if (same != 0U)
    {
        return;
    }

    i = 0U;
    while ((state[i] != '\0') && (i < (uint16_t)(STATE_SIZE - 1U)))
    {
        lastStateText[i] = state[i];
        i++;
    }
    lastStateText[i] = '\0';

    copyAscii(stateBuffer, STATE_SIZE, state);
    txtState.invalidate();
}

void BloodPressureScreenView::updateHint(const char* hint)
{
    copyAscii(hintBuffer, HINT_SIZE, hint);
    txtHint.invalidate();
}

void BloodPressureScreenView::updateResult(float sbp, float dbp, uint8_t hr, uint8_t valid)
{
    if (valid != 0U)
    {
        touchgfx::Unicode::snprintf(sbpBuffer, RESULT_SIZE, "%03d", (int)(sbp + 0.5f));
        touchgfx::Unicode::snprintf(dbpBuffer, RESULT_SIZE, "%03d", (int)(dbp + 0.5f));
        touchgfx::Unicode::snprintf(hrBuffer, RESULT_SIZE, "%03u", (unsigned int)hr);
    }
    else
    {
        touchgfx::Unicode::snprintf(sbpBuffer, RESULT_SIZE, "000");
        touchgfx::Unicode::snprintf(dbpBuffer, RESULT_SIZE, "000");
        touchgfx::Unicode::snprintf(hrBuffer, RESULT_SIZE, "000");
    }

    txtSBP.invalidate();
    txtDBP.invalidate();
    txtHR.invalidate();
}

void BloodPressureScreenView::clearBPGraph()
{
    BPgraph.clear();
    BPgraph.invalidate();
}

void BloodPressureScreenView::addBPGraphPoint(float pressure)
{
    if (pressure < 0.0f)
    {
        pressure = 0.0f;
    }
    else if (pressure > 250.0f)
    {
        pressure = 250.0f;
    }

    BPgraph.addDataPoint(pressure);
}
