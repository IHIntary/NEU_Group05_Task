/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : max30102.c
 * @brief          : 血氧传感器
 ******************************************************************************
 * @attention
 * 1.要宏定义 ARM_MATH_CM7,__FPU_PRESENT
 * 2.打开DSP
 * 3.main函数定义如下全局变量
 *	uint8_t max30102_int_flag = 0; // 中断标志
 *	float ppg_data_cache_RED[CACHE_NUMS] = {0}; // 缓存区
 *	float ppg_data_cache_IR[CACHE_NUMS] = {0};  // 缓存区
 *	uint16_t cache_counter = 0; // 缓存计数器
 * 
 ******************************************************************************
 */
/* USER CODE END Header */
#include "max30102.h"
#include "app_sensor_config.h"
#include "max30102_fir.h"
#include "stdio.h"

#include "algorithm.h"
#include "max30102_fir.h"

volatile uint8_t max30102_int_flag = 0; // 中断标志
float ppg_data_cache_RED[CACHE_NUMS] = {0}; // 缓存区
float ppg_data_cache_IR[CACHE_NUMS] = {0};  // 缓存区
uint16_t cache_counter = 0; // 缓存计数器

#ifdef _ALGORITHM_
uint32_t ppg_data_cache_RED[CACHE_NUMS] = {0}; // 缓存区
uint32_t ppg_data_cache_IR[CACHE_NUMS] = {0};  // 缓存区
#endif

/**
 * @brief IIC 写入
 * @retval None
 */
void max30102_i2c_write(uint8_t reg_adder, uint8_t data)
{
	uint8_t transmit_data[2];
	transmit_data[0] = reg_adder;
	transmit_data[1] = data;
	i2c_transmit(transmit_data, 2);
}

/**
 * @brief IIC 读取
 * @retval None
 */
void max30102_i2c_read(uint8_t reg_adder, uint8_t *pdata, uint8_t data_size)
{
	AppI2c3_MemRead8(I2C_WRITE_ADDR, reg_adder, pdata, data_size, 10);
}

/**
 * @brief max30102初始化
 * @retval None
 */
void max30102_init(void)
{
	uint8_t data;

	max30102_i2c_write(MODE_CONFIGURATION, 0x40); // reset the device

	max30102_i2c_write(INTERRUPT_ENABLE1, 0xE0);
	max30102_i2c_write(INTERRUPT_ENABLE2, 0x00); // interrupt enable: FIFO almost full flag, new FIFO Data Ready,
												 //                   ambient light cancellation overflow, power ready flag,
												 //					  internal temperature ready flag

	max30102_i2c_write(FIFO_WR_POINTER, 0x00);
	max30102_i2c_write(FIFO_OV_COUNTER, 0x00);
	max30102_i2c_write(FIFO_RD_POINTER, 0x00); // clear the pointer

	max30102_i2c_write(FIFO_CONFIGURATION, 0x4F); // FIFO configuration: sample averaging(1),FIFO rolls on full(0), 
	                                              //                     FIFO almost full value(15 empty data samples 
	                                              //                     when interrupt is issued)

	max30102_i2c_write(MODE_CONFIGURATION, 0x03); // MODE configuration:SpO2 mode

	max30102_i2c_write(SPO2_CONFIGURATION, 0x26); // SpO2 configuration:ADC range 4096nA,sample rate control:100Hz,
	                                              //                    LED pulse width:215 us

	max30102_i2c_write(LED1_PULSE_AMPLITUDE, 0x2f); // RED LED
	max30102_i2c_write(LED2_PULSE_AMPLITUDE, 0x2f); // IR LED current

	max30102_i2c_write(TEMPERATURE_CONFIG, 0x01); // temp

	max30102_i2c_read(INTERRUPT_STATUS1, &data, 1);
	max30102_i2c_read(INTERRUPT_STATUS2, &data, 1); // clear status
}

/**
 * @brief fifo区读取
 * @param output_data
 * @retval None
 */
void max30102_fifo_read(float *output_data)
{
	uint8_t receive_data[6];
	uint32_t data[2];
	max30102_i2c_read(FIFO_DATA, receive_data, 6);
	data[0] = ((receive_data[0] << 16 | receive_data[1] << 8 | receive_data[2]) & 0x03ffff);
	data[1] = ((receive_data[3] << 16 | receive_data[4] << 8 | receive_data[5]) & 0x03ffff);
	*output_data = data[1];
	*(output_data + 1) = data[0];
}

/**
 * @brief 获取心率
 * @param input_data cache_nums(缓存区的最大数字)
 * @retval (uint16_t)心率
 */
uint16_t max30102_getHeartRate(float *input_data, uint16_t cache_nums)
{
	float input_data_sum_aver = 0.0f;
	float input_data_max;
	float input_data_min;
	uint16_t i;
	uint16_t first_crossing = 0U;
	uint16_t period_samples = 0U;
	uint16_t min_period_samples;
	uint16_t max_period_samples;
	uint32_t sample_period_ms = SENSOR_PPG_SAMPLE_PERIOD_MS;
	uint32_t heart_rate;

	if ((input_data == 0) || (cache_nums < 2U) || (sample_period_ms == 0U))
	{
		return 0U;
	}

	min_period_samples = (uint16_t)((60000UL + ((uint32_t)180U * sample_period_ms) - 1UL) /
	                                ((uint32_t)180U * sample_period_ms));
	max_period_samples = (uint16_t)(60000UL / ((uint32_t)40U * sample_period_ms));

	if (min_period_samples < 1U)
	{
		min_period_samples = 1U;
	}
	if (max_period_samples >= cache_nums)
	{
		max_period_samples = (uint16_t)(cache_nums - 1U);
	}
	if (min_period_samples > max_period_samples)
	{
		return 0U;
	}

	input_data_max = input_data[0];
	input_data_min = input_data[0];

	for (i = 0; i < cache_nums; i++)
	{
		float value = input_data[i];

		input_data_sum_aver += value;
		if (value > input_data_max)
		{
			input_data_max = value;
		}
		if (value < input_data_min)
		{
			input_data_min = value;
		}
	}
	input_data_sum_aver = input_data_sum_aver / cache_nums;

	if ((input_data_max - input_data_min) < 1000.0f)
	{
		return 0U;
	}

	for (i = 0; i + 1U < cache_nums; i++)
	{
		if ((input_data[i] > input_data_sum_aver) && (input_data[i + 1U] <= input_data_sum_aver))
		{
			first_crossing = i;
			break;
		}
	}

	if (i + 1U >= cache_nums)
	{
		return 0U;
	}

	i++;
	for (; i + 1U < cache_nums; i++)
	{
		if ((input_data[i] > input_data_sum_aver) && (input_data[i + 1U] <= input_data_sum_aver))
		{
			period_samples = (uint16_t)(i - first_crossing);
			if ((period_samples >= min_period_samples) && (period_samples <= max_period_samples))
			{
				heart_rate = (60000UL + (((uint32_t)period_samples * sample_period_ms) / 2UL)) /
				             ((uint32_t)period_samples * sample_period_ms);
				if ((heart_rate >= 40UL) && (heart_rate <= 180UL))
				{
					return (uint16_t)heart_rate;
				}
				return 0U;
			}
		}
	}

	return 0U;
}

/**
 * @brief 获取血氧
 * @param input_data red_input_data cache_nums(缓存区的最大数字)
 * @retval (float)血氧
 */
float max30102_getSpO2(float *ir_input_data, float *red_input_data, uint16_t cache_nums)
{
	float ir_max = *ir_input_data, ir_min = *ir_input_data;
	float red_max = *red_input_data, red_min = *red_input_data;
	float R;
	float ret;
	uint16_t i;
	for (i = 1; i < cache_nums; i++)
	{
		if (ir_max < *(ir_input_data + i))
		{
			ir_max = *(ir_input_data + i);
		}
		if (ir_min > *(ir_input_data + i))
		{
			ir_min = *(ir_input_data + i);
		}
		if (red_max < *(red_input_data + i))
		{
			red_max = *(red_input_data + i);
		}
		if (red_min > *(red_input_data + i))
		{
			red_min = *(red_input_data + i);
		}
	}

	if (((ir_max - ir_min) == 0.0f) || ((red_max + red_min) == 0.0f))
	{
		return 0.0f;
	}
	
	R = ((ir_max + ir_min) * (red_max - red_min)) / ((red_max + red_min) * (ir_max - ir_min));
	
	ret = ((-45.060f) * R * R + 30.354f * R + 94.845f);
	
	if(ret<0 || ret>100)
		ret = 0.0f;
	
	return ret;
}

/**
 * @brief MAX30102服务函数
 * @param HeartRate(心率) SpO2(血氧) max30102_data fir_output
 * @retval (uint8_t)MAX30102_DATA_OK:结束读取  (uint8_t)!MAX30102_DATA_OK:还在读取
 */
uint8_t MAX30102_Get_DATA(uint16_t *HeartRate,float *SpO2)
{
	float max30102_data[2];
	float fir_output[2];
		
	if (max30102_int_flag) // 中断信号产生
	{
		max30102_int_flag = 0;
		max30102_fifo_read(max30102_data); // 读取数据	
		
		
		if ((max30102_data[0] > PPG_DATA_THRESHOLD) && (max30102_data[1] > PPG_DATA_THRESHOLD)) // 大于阈值，说明传感器有接触
		{
			ir_max30102_fir(&max30102_data[0], &fir_output[0]);  // 滤波
			red_max30102_fir(&max30102_data[1], &fir_output[1]); // 滤波
			
			ppg_data_cache_IR[cache_counter] = fir_output[0];
			ppg_data_cache_RED[cache_counter] = fir_output[1];
			
			cache_counter++;
		}
		else // 小于阈值
		{
			cache_counter = 0;
		}
		
		if (cache_counter >= CACHE_NUMS) // 收集满了数据
		{			
			*HeartRate = max30102_getHeartRate(ppg_data_cache_IR, CACHE_NUMS);
			*SpO2 = max30102_getSpO2(ppg_data_cache_IR, ppg_data_cache_RED, CACHE_NUMS);

			cache_counter = 0;			
			
			return MAX30102_DATA_OK;
		}
	}
	return !MAX30102_DATA_OK;
}

/**
 * @brief MAX30102服务函数
 * @param HeartRate(心率) SpO2(血氧) max30102_data fir_output
 * @retval (uint8_t)MAX30102_DATA_OK:结束读取  (uint8_t)!MAX30102_DATA_OK:还在读取
 */
#ifdef _ALGORITHM_

#endif

/**
 * @brief MAX30102输入引脚外部中断触发
 * @param GPIO_Pin
 * @attention cubemx配置下降沿 上拉 允许中断
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == MAX30102_INT_Pin)
  {
    max30102_int_flag = 1;
  }
}

/**
 * @brief MAX30102服务函数
 * @retval (uint8_t)MAX30102_DATA_OK:结束读取  (uint8_t)!MAX30102_DATA_OK:还在读取
 */
uint8_t MAX30102_Get_RAW_DATA(float max30102_data[2],float fir_output[2])
{
	if (max30102_int_flag) // 中断信号产生
	{
		max30102_int_flag = 0;
		
		max30102_fifo_read(max30102_data); // 读取数据

		if ((max30102_data[0] > PPG_DATA_THRESHOLD) && (max30102_data[1] > PPG_DATA_THRESHOLD)) // 大于阈值，说明传感器有接触
		{
			ir_max30102_fir(&max30102_data[0], &fir_output[0]);  // 滤波
			red_max30102_fir(&max30102_data[1], &fir_output[1]); // 滤波
			
			return MAX30102_DATA_OK;
		}
	}
	return !MAX30102_DATA_OK;
}










