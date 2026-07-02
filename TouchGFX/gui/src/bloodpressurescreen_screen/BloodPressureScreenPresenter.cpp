#include <gui/bloodpressurescreen_screen/BloodPressureScreenView.hpp>
#include <gui/bloodpressurescreen_screen/BloodPressureScreenPresenter.hpp>
#include <gui/model/Model.hpp>

BloodPressureScreenPresenter::BloodPressureScreenPresenter(BloodPressureScreenView& v)
    : view(v),
      graphRunning(0U),
      graphTickDivider(0U),
			uiTickDivider(0U)
{

}

void BloodPressureScreenPresenter::activate()
{
    graphRunning = 0U;
    graphTickDivider = 0U;
		uiTickDivider = 0U;
    view.clearBPGraph();
}

void BloodPressureScreenPresenter::deactivate()
{

}

void BloodPressureScreenPresenter::startBPMeasure()
{
    graphRunning = 1U;
    graphTickDivider = 0U;
		uiTickDivider = 0U;
    view.clearBPGraph();
    model->startBPMeasure();
    model->beep(60U);
}

void BloodPressureScreenPresenter::resetBPMeasure()
{
    graphRunning = 0U;
    graphTickDivider = 0U;
		uiTickDivider = 0U;
    view.clearBPGraph();
    model->resetBPMeasure();
    model->beep(60U);
}

void BloodPressureScreenPresenter::bpDataUpdated(float pressure, float speed, const char* state, const char* hint,
                                                 float sbp, float dbp, uint8_t hr, uint8_t valid)
{
    uint8_t terminal = ((valid != 0U) ||
    ((state != 0) && (state[0] == 'E') && (state[1] == 'R') && (state[2] == 'R')));

		uiTickDivider++;
		if ((uiTickDivider >= 6U) || (terminal != 0U))
		{
				uiTickDivider = 0U;

				view.updatePressure(pressure);
				view.updateSpeed(speed);
				view.updateState(state);
				view.updateHint(hint);
				view.updateResult(sbp, dbp, hr, valid);
		}

    if (graphRunning != 0U)
    {
        graphTickDivider++;
        if (graphTickDivider >= 8U)
        {
            graphTickDivider = 0U;
            view.addBPGraphPoint(pressure);
        }
    }

    if ((valid != 0U) ||
        ((state != 0) && (state[0] == 'E') && (state[1] == 'R') && (state[2] == 'R')))
    {
        graphRunning = 0U;
    }
}
