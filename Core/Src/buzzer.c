#include "buzzer.h"
#include "app_i2c_bus.h"
#include "cmsis_os2.h"

#define BUZZER_I2C_ADDR 0x40
#define BUZZER_I2C_TIMEOUT_MS 50U

void buzzer(uint16_t b_time)
{
    uint8_t beep;

    beep = 0x00;
    (void)AppI2c2_MasterTransmit(BUZZER_I2C_ADDR, &beep, 1, BUZZER_I2C_TIMEOUT_MS);

    osDelay(b_time);

    beep = 0x01;
    (void)AppI2c2_MasterTransmit(BUZZER_I2C_ADDR, &beep, 1, BUZZER_I2C_TIMEOUT_MS);
}
