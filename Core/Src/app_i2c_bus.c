#include "app_i2c_bus.h"
#include "i2c.h"
#include "cmsis_os2.h"

static osMutexId_t Group05_I2C2Mtx;
static osMutexId_t Group05_I2C3Mtx;

static const osMutexAttr_t Group05_I2C2Mtx_attributes = {
    .name = "Group05_I2C2Mtx",
};

static const osMutexAttr_t Group05_I2C3Mtx_attributes = {
    .name = "Group05_I2C3Mtx",
};

void AppI2cBus_Init(void)
{
    if (Group05_I2C2Mtx == NULL)
    {
        Group05_I2C2Mtx = osMutexNew(&Group05_I2C2Mtx_attributes);
    }

    if (Group05_I2C3Mtx == NULL)
    {
        Group05_I2C3Mtx = osMutexNew(&Group05_I2C3Mtx_attributes);
    }
}

static uint8_t AppI2c2_Lock(uint32_t timeout)
{
    osKernelState_t kernelState = osKernelGetState();

    if ((kernelState != osKernelRunning) && (kernelState != osKernelReady))
    {
        return 1U;
    }

    AppI2cBus_Init();
    if (Group05_I2C2Mtx == NULL)
    {
        return 0U;
    }

    return (osMutexAcquire(Group05_I2C2Mtx, timeout) == osOK) ? 1U : 0U;
}

static void AppI2c2_Unlock(void)
{
    osKernelState_t kernelState = osKernelGetState();

    if (((kernelState == osKernelRunning) || (kernelState == osKernelReady)) && (Group05_I2C2Mtx != NULL))
    {
        (void)osMutexRelease(Group05_I2C2Mtx);
    }
}

static uint8_t AppI2c3_Lock(uint32_t timeout)
{
    osKernelState_t kernelState = osKernelGetState();

    if ((kernelState != osKernelRunning) && (kernelState != osKernelReady))
    {
        return 1U;
    }

    AppI2cBus_Init();
    if (Group05_I2C3Mtx == NULL)
    {
        return 0U;
    }

    return (osMutexAcquire(Group05_I2C3Mtx, timeout) == osOK) ? 1U : 0U;
}

static void AppI2c3_Unlock(void)
{
    osKernelState_t kernelState = osKernelGetState();

    if (((kernelState == osKernelRunning) || (kernelState == osKernelReady)) && (Group05_I2C3Mtx != NULL))
    {
        (void)osMutexRelease(Group05_I2C3Mtx);
    }
}

HAL_StatusTypeDef AppI2c2_MasterTransmit(uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (AppI2c2_Lock(timeout) == 0U)
    {
        return HAL_BUSY;
    }

    status = HAL_I2C_Master_Transmit(&hi2c2, devAddress, data, size, timeout);
    AppI2c2_Unlock();

    return status;
}

HAL_StatusTypeDef AppI2c3_MasterTransmit(uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (AppI2c3_Lock(timeout) == 0U)
    {
        return HAL_BUSY;
    }

    status = HAL_I2C_Master_Transmit(&hi2c3, devAddress, data, size, timeout);
    AppI2c3_Unlock();

    return status;
}

HAL_StatusTypeDef AppI2c3_MasterReceive(uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (AppI2c3_Lock(timeout) == 0U)
    {
        return HAL_BUSY;
    }

    status = HAL_I2C_Master_Receive(&hi2c3, devAddress, data, size, timeout);
    AppI2c3_Unlock();

    return status;
}

HAL_StatusTypeDef AppI2c3_MemRead8(uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (AppI2c3_Lock(timeout) == 0U)
    {
        return HAL_BUSY;
    }

    status = HAL_I2C_Master_Transmit(&hi2c3, devAddress, &regAddress, 1U, timeout);
    if (status == HAL_OK)
    {
        status = HAL_I2C_Master_Receive(&hi2c3, devAddress | 0x01U, data, size, timeout);
    }

    AppI2c3_Unlock();

    return status;
}

HAL_StatusTypeDef AppI2c2_MasterReceive(uint16_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (AppI2c2_Lock(timeout) == 0U)
    {
        return HAL_BUSY;
    }

    status = HAL_I2C_Master_Receive(&hi2c2, devAddress, data, size, timeout);
    AppI2c2_Unlock();

    return status;
}

HAL_StatusTypeDef AppI2c2_MemRead8(uint16_t devAddress, uint8_t regAddress, uint8_t *data, uint16_t size, uint32_t timeout)
{
    HAL_StatusTypeDef status;

    if (AppI2c2_Lock(timeout) == 0U)
    {
        return HAL_BUSY;
    }

    status = HAL_I2C_Master_Transmit(&hi2c2, devAddress, &regAddress, 1U, timeout);
    if (status == HAL_OK)
    {
        status = HAL_I2C_Master_Receive(&hi2c2, devAddress | 0x01U, data, size, timeout);
    }

    AppI2c2_Unlock();

    return status;
}
