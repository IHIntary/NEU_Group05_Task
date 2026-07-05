#include "app_input_service.h"
#include "cmsis_compiler.h"
#include "main.h"

__WEAK void remote_init(void)
{
}

__WEAK uint8_t remote_scan(void)
{
    return 0U;
}

static uint8_t lastKey2Pressed;
static uint8_t key2StableTicks;

void AppInputService_Init(void)
{
    lastKey2Pressed = 0U;
    key2StableTicks = 0U;
    remote_init();
}

uint8_t AppInputService_ScanRemoteKey(void)
{
    return remote_scan();
}

uint8_t AppInputService_PollKey2Pressed(void)
{
    uint8_t key2Pressed = (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) ? 1U : 0U;

    if (key2Pressed != lastKey2Pressed)
    {
        key2StableTicks++;
        if (key2StableTicks >= 2U)
        {
            lastKey2Pressed = key2Pressed;
            key2StableTicks = 0U;
            return key2Pressed;
        }
    }
    else
    {
        key2StableTicks = 0U;
    }

    return 0U;
}
