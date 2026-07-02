#include <gui/homescreen_screen/HomeScreenView.hpp>

HomeScreenView::HomeScreenView()
{

}

void HomeScreenView::setupScreen()
{
    HomeScreenViewBase::setupScreen();
		txtChipTemp.setWildcard(chipTempBuffer);
		updateChipTemp(0.0f, 0U);
}

void HomeScreenView::tearDownScreen()
{
    HomeScreenViewBase::tearDownScreen();
}

void HomeScreenView::updateChipTemp(float tempC, uint8_t valid)
{
    int tenths;

    if (valid == 0U)
    {
        touchgfx::Unicode::snprintf(chipTempBuffer, CHIP_TEMP_SIZE, "--.-");
    }
    else
    {
        tenths = (int)((tempC * 10.0f) + 0.5f);
        touchgfx::Unicode::snprintf(chipTempBuffer, CHIP_TEMP_SIZE, "%d.%d", tenths / 10, tenths % 10);
    }

    txtChipTemp.invalidate();
}