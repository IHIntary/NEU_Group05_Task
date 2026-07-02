/* USER CODE BEGIN Header */
#include "key.h"

/* USER CODE BEGIN 0 */
uint8_t key_scan(uint8_t mode)
{
    (void)mode;

    if ((HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) ||
        (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) ||
        (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) ||
        (HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin) == GPIO_PIN_SET))
    {
        HAL_Delay(10);

        if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
        {
            return KEY0_PRES;
        }
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
        {
            return KEY1_PRES;
        }
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
        {
            return KEY2_PRES;
        }
        if (HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin) == GPIO_PIN_SET)
        {
            return WKUP_PRES;
        }
    }

    return 0U;
}
/* USER CODE END 0 */
