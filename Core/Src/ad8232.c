#include "ad8232.h"
#include "adc.h"

#define AD8232_ADC_MAX_VALUE       4095.0f
#define AD8232_ADC_REF_VOLTAGE     3.3f
static uint32_t ad8232LastAdcError = 0U;

static void AD8232_ConfigChannel(uint32_t channel, uint32_t samplingTime)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = samplingTime;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

uint16_t AD8232_ReadRaw(void)
{
    uint16_t raw = 0U;
    HAL_StatusTypeDef status;

    AD8232_ConfigChannel(ADC_CHANNEL_11, ADC_SAMPLETIME_144CYCLES);

    status = HAL_ADC_Start(&hadc1);
    if (status == HAL_OK)
    {
        status = HAL_ADC_PollForConversion(&hadc1, 10U);
        if (status == HAL_OK)
        {
            raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
            ad8232LastAdcError = 0U;
        }
        else
        {
            ad8232LastAdcError = 0x02000000U | (uint32_t)status | HAL_ADC_GetError(&hadc1);
        }
    }
    else
    {
        ad8232LastAdcError = 0x01000000U | (uint32_t)status | HAL_ADC_GetError(&hadc1);
    }

    (void)HAL_ADC_Stop(&hadc1);

    return raw;
}

float AD8232_ReadVoltage(void)
{
    return ((float)AD8232_ReadRaw() * AD8232_ADC_REF_VOLTAGE) / AD8232_ADC_MAX_VALUE;
}

uint8_t AD8232_IsLeadsOff(void)
{
    if ((AD8232_ReadLOP() != 0U) || (AD8232_ReadLON() != 0U))
    {
        return 1U;
    }

    return 0U;
}

uint8_t AD8232_ReadLOP(void)
{
    return (HAL_GPIO_ReadPin(AD8232_LOP_GPIO_Port, AD8232_LOP_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

uint8_t AD8232_ReadLON(void)
{
    return (HAL_GPIO_ReadPin(AD8232_LON_GPIO_Port, AD8232_LON_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

uint32_t AD8232_GetLastAdcError(void)
{
    return ad8232LastAdcError;
}
