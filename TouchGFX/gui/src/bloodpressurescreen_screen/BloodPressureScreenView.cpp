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

static float mmHgToPa(float mmHg)
{
    return mmHg * 133.322f;
}

BloodPressureScreenView::BloodPressureScreenView()
    : displayPa(0U),
      lastPressureMmHg(0.0f),
      lastSpeedMmHg(0.0f),
      lastSbpMmHg(0.0f),
      lastDbpMmHg(0.0f),
      lastHr(0U),
      lastResultValid(0U)
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

void BloodPressureScreenView::unitToggleClicked()
{
    displayPa = (displayPa == 0U) ? 1U : 0U;

    lastPressureValue = -1;
    lastSpeedTenths = -1;

    updatePressure(lastPressureMmHg);
    updateSpeed(lastSpeedMmHg);
    updateResult(lastSbpMmHg, lastDbpMmHg, lastHr, lastResultValid);
}

void BloodPressureScreenView::updatePressure(float pressure)
{
    int value;

    if (pressure < 0.0f)
    {
        pressure = 0.0f;
    }

    lastPressureMmHg = pressure;

    if (displayPa != 0U)
    {
        value = (int)(mmHgToPa(pressure) + 0.5f);
        touchgfx::Unicode::snprintf(pressureBuffer, PRESSURE_SIZE, "%d Pa", value);
    }
    else
    {
        value = (int)(pressure + 0.5f);
        touchgfx::Unicode::snprintf(pressureBuffer, PRESSURE_SIZE, "%03d mmHg", value);
    }

    if (value == lastPressureValue)
    {
        return;
    }

    lastPressureValue = value;
    txtPressure.invalidate();
}

void BloodPressureScreenView::updateSpeed(float speed)
{
    int tenths;

    if (speed < 0.0f)
    {
        speed = 0.0f;
    }

    lastSpeedMmHg = speed;

    if (displayPa != 0U)
    {
        tenths = (int)((mmHgToPa(speed) * 10.0f) + 0.5f);
        touchgfx::Unicode::snprintf(speedBuffer, SPEED_SIZE, "%d.%d Pa/s", tenths / 10, tenths % 10);
    }
    else
    {
        tenths = (int)((speed * 10.0f) + 0.5f);
        touchgfx::Unicode::snprintf(speedBuffer, SPEED_SIZE, "%d.%d mmHg/s", tenths / 10, tenths % 10);
    }

    if (tenths == lastSpeedTenths)
    {
        return;
    }

    lastSpeedTenths = tenths;
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
    lastSbpMmHg = sbp;
    lastDbpMmHg = dbp;
    lastHr = hr;
    lastResultValid = valid;

    if (valid != 0U)
    {
        if (displayPa != 0U)
        {
            touchgfx::Unicode::snprintf(sbpBuffer, RESULT_SIZE, "%d Pa", (int)(mmHgToPa(sbp) + 0.5f));
            touchgfx::Unicode::snprintf(dbpBuffer, RESULT_SIZE, "%d Pa", (int)(mmHgToPa(dbp) + 0.5f));
        }
        else
        {
            touchgfx::Unicode::snprintf(sbpBuffer, RESULT_SIZE, "%03d mmHg", (int)(sbp + 0.5f));
            touchgfx::Unicode::snprintf(dbpBuffer, RESULT_SIZE, "%03d mmHg", (int)(dbp + 0.5f));
        }

        touchgfx::Unicode::snprintf(hrBuffer, RESULT_SIZE, "%03u", (unsigned int)hr);
    }
    else
    {
        if (displayPa != 0U)
        {
            touchgfx::Unicode::snprintf(sbpBuffer, RESULT_SIZE, "0 Pa");
            touchgfx::Unicode::snprintf(dbpBuffer, RESULT_SIZE, "0 Pa");
        }
        else
        {
            touchgfx::Unicode::snprintf(sbpBuffer, RESULT_SIZE, "000 mmHg");
            touchgfx::Unicode::snprintf(dbpBuffer, RESULT_SIZE, "000 mmHg");
        }

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
