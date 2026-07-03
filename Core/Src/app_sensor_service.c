#include "app_sensor_service.h"
#include "app_sensor_config.h"
#include "ad8232.h"
#include "adc.h"
#include "i2c.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include "max30102.h"
#include "bp_service.h"
#include "app_buzzer_service.h"
#include "sh3001.h"
#include <stdio.h>
#include <string.h>

static SensorData_t g_sensorData;
static osMutexId_t Group05_SensMtx;

static const osMutexAttr_t Group05_SensMtx_attributes = {
    .name = "Group05_SensMtx",
};

static float irBuffer[SENSOR_PPG_GROUP_SAMPLES];
static float redBuffer[SENSOR_PPG_GROUP_SAMPLES];
static uint16_t ppgIndex = 0;
static uint8_t ppgGroupIndex = 0;
static uint32_t ppgHeartRateSum = 0;
static float ppgSpo2Sum = 0.0f;

static uint32_t lastEcgLogTick = 0;
static uint32_t lastPressureLogTick = 0;
static uint32_t lastPulseLogTick = 0;

static float pressureFilteredRaw = 0.0f;
static uint8_t pressureFilterReady = 0U;
static uint8_t pressureStartLogPending = 0U;

static float ecgHighPassState = 0.0f;
static uint16_t ecgPrevRaw = SENSOR_ECG_FILTER_BASELINE_RAW;
static uint16_t ecgAvgBuffer[SENSOR_ECG_FILTER_AVG_SIZE];
static uint8_t ecgAvgIndex = 0U;
static uint8_t ecgAvgCount = 0U;
static uint32_t ecgAvgSum = 0U;

static uint8_t SensorService_IsPulseResultValid(uint16_t heartRate, float spo2)
{
    return ((heartRate >= 40U) &&
            (heartRate <= 180U) &&
            (spo2 >= 70.0f) &&
            (spo2 <= 100.0f)) ? 1U : 0U;
}

static void SensorService_ResetEcgFilter(uint16_t raw)
{
    uint8_t i;

    ecgHighPassState = 0.0f;
    ecgPrevRaw = raw;
    ecgAvgIndex = 0U;
    ecgAvgCount = SENSOR_ECG_FILTER_AVG_SIZE;
    ecgAvgSum = 0U;

    for (i = 0U; i < SENSOR_ECG_FILTER_AVG_SIZE; i++)
    {
        ecgAvgBuffer[i] = SENSOR_ECG_FILTER_BASELINE_RAW;
        ecgAvgSum += SENSOR_ECG_FILTER_BASELINE_RAW;
    }
}

static uint16_t SensorService_FilterEcg(uint16_t raw)
{
    const float highPassAlpha = 0.98f;
    int32_t centered;
    uint16_t highPassed;
    uint16_t averaged;

    ecgHighPassState = highPassAlpha * (ecgHighPassState + (float)raw - (float)ecgPrevRaw);
    ecgPrevRaw = raw;

    centered = (int32_t)SENSOR_ECG_FILTER_BASELINE_RAW + (int32_t)ecgHighPassState;
    if (centered < 0)
    {
        highPassed = 0U;
    }
    else if (centered > (int32_t)SENSOR_ADC_MAX_VALUE)
    {
        highPassed = SENSOR_ADC_MAX_VALUE;
    }
    else
    {
        highPassed = (uint16_t)centered;
    }

    if (ecgAvgCount < SENSOR_ECG_FILTER_AVG_SIZE)
    {
        ecgAvgCount++;
    }
    else
    {
        ecgAvgSum -= ecgAvgBuffer[ecgAvgIndex];
    }

    ecgAvgBuffer[ecgAvgIndex] = highPassed;
    ecgAvgSum += highPassed;
    ecgAvgIndex++;
    if (ecgAvgIndex >= SENSOR_ECG_FILTER_AVG_SIZE)
    {
        ecgAvgIndex = 0U;
    }

    averaged = (uint16_t)((ecgAvgSum + (ecgAvgCount / 2U)) / ecgAvgCount);
    return averaged;
}

static uint16_t ReadAdcChannelWithSampleTime(uint32_t channel, uint32_t samplingTime)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t value = 0;

    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = samplingTime;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        return 0;
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return 0;
    }

    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }

    HAL_ADC_Stop(&hadc1);
    return value;
}

static uint16_t ReadAdcChannel(uint32_t channel)
{
    return ReadAdcChannelWithSampleTime(channel, ADC_SAMPLETIME_144CYCLES);
}

#define SENSOR_TEMP_V25        0.76f
#define SENSOR_TEMP_AVG_SLOPE  0.0025f
#define SENSOR_TEMP_PERIOD_MS  1000U

#define SENSOR_IMU_SAMPLE_PERIOD_MS         20U
#define SENSOR_IMU_INIT_RETRY_PERIOD_MS     500U
#define SENSOR_IMU_ACC_LSB_PER_G            2048L
#define SENSOR_IMU_AXIS_ALARM_THRESHOLD_MG  50L

static int16_t imuBaselineMg[3] = {0};
static uint8_t imuBaselineValid = 0U;
static uint8_t imuRecalibrateBaseline = 1U;

static int16_t SensorService_AccRawToMg(short raw)
{
    int32_t mg = ((int32_t)raw * 1000L) / SENSOR_IMU_ACC_LSB_PER_G;

    if (mg > 32767L)
    {
        return 32767;
    }

    if (mg < -32768L)
    {
        return -32768;
    }

    return (int16_t)mg;
}

static uint8_t SensorService_IsAxisDeltaOverThreshold(int16_t value, int16_t baseline)
{
    int32_t delta = (int32_t)value - (int32_t)baseline;
    if (delta < 0)
    {
        delta = -delta;
    }

    return (delta > SENSOR_IMU_AXIS_ALARM_THRESHOLD_MG) ? 1U : 0U;
}

static void SensorService_UpdateChipTemp(void)
{
    uint16_t raw;
    float vsense;
    float tempC;

    raw = ReadAdcChannelWithSampleTime(ADC_CHANNEL_TEMPSENSOR, ADC_SAMPLETIME_480CYCLES);
    vsense = ((float)raw * 3.3f) / 4095.0f;
    tempC = ((vsense - SENSOR_TEMP_V25) / SENSOR_TEMP_AVG_SLOPE) + 25.0f;

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.chipTempC = tempC;
    g_sensorData.chipTempValid = 1U;
    osMutexRelease(Group05_SensMtx);
}

void SensorService_Init(void)
{
    memset(&g_sensorData, 0, sizeof(g_sensorData));
    Group05_SensMtx = osMutexNew(&Group05_SensMtx_attributes);
    SensorService_ResetEcgFilter(SENSOR_ECG_FILTER_BASELINE_RAW);
    g_sensorData.ecgFiltered = SENSOR_ECG_FILTER_BASELINE_RAW;

    max30102_init();
    g_sensorData.max30102Ready = 1U;

}

void SensorService_GetData(SensorData_t *out)
{
    if (out == NULL)
    {
        return;
    }

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    *out = g_sensorData;
    osMutexRelease(Group05_SensMtx);
}

void SensorService_SetEcgRunning(uint8_t running)
{
    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.ecgRunning = running ? 1U : 0U;
    if (running)
    {
        SensorService_ResetEcgFilter(g_sensorData.ecgRaw);
        g_sensorData.ecgFiltered = SENSOR_ECG_FILTER_BASELINE_RAW;
    }
    osMutexRelease(Group05_SensMtx);
}

void SensorService_SetPulseRunning(uint8_t running)
{
    uint8_t logStart = 0U;

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.pulseRunning = running ? 1U : 0U;
    if (running)
    {
        g_sensorData.pulseValidCount = 0;
        g_sensorData.pulseProgressPercent = 0;
        g_sensorData.pulseDone = 0;
        g_sensorData.pulseDataValid = 0;
        g_sensorData.ppgIr = 0;
        g_sensorData.ppgRed = 0;
        g_sensorData.heartRate = 0;
        g_sensorData.spo2 = 0.0f;
        ppgIndex = 0;
        ppgGroupIndex = 0;
        ppgHeartRateSum = 0;
        ppgSpo2Sum = 0.0f;
        lastPulseLogTick = 0U;
        logStart = 1U;
    }
    osMutexRelease(Group05_SensMtx);

    if (logStart != 0U)
    {
        printf("[MAX30102] start\r\n");
    }
}

void SensorService_SetPressureRunning(uint8_t running)
{
    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.pressureRunning = running ? 1U : 0U;
		if (running)
		{
				pressureFilteredRaw = 0.0f;
				pressureFilterReady = 0U;
				pressureStartLogPending = 1U;
		}
    osMutexRelease(Group05_SensMtx);
}

void SensorService_SetImuRunning(uint8_t running)
{
    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.imuRunning = running ? 1U : 0U;
    g_sensorData.imuDataValid = 0U;

    if (running != 0U)
    {
        imuRecalibrateBaseline = 1U;
    }
    else
    {
        g_sensorData.imuFallDetected = 0U;
        g_sensorData.imuAlarmActive = 0U;
    }

    osMutexRelease(Group05_SensMtx);
}

void SensorService_ClearImuAlarm(void)
{
    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.imuFallDetected = 0U;
    g_sensorData.imuAlarmActive = 0U;
    imuRecalibrateBaseline = 1U;
    osMutexRelease(Group05_SensMtx);
}

static void SensorService_TryInitImu(void)
{
    uint8_t ready;

    if (sh3001_init() == SH3001_TRUE)
    {
        ready = 1U;
    }
    else
    {
        ready = 0U;
    }

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.imuReady = ready;
    g_sensorData.imuDataValid = 0U;
    imuBaselineValid = 0U;
    imuRecalibrateBaseline = 1U;
    osMutexRelease(Group05_SensMtx);
}

static void SensorService_UpdateImu(void)
{
    short accRaw[3] = {0};
    short gyroRaw[3] = {0};
    int16_t accMg[3];
    uint8_t thresholdExceeded;
    uint8_t latchAlarm = 0U;

    sh3001_get_imu_compdata(accRaw, gyroRaw);

    accMg[0] = SensorService_AccRawToMg(accRaw[0]);
    accMg[1] = SensorService_AccRawToMg(accRaw[1]);
    accMg[2] = SensorService_AccRawToMg(accRaw[2]);

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    if ((imuBaselineValid == 0U) || (imuRecalibrateBaseline != 0U))
    {
        imuBaselineMg[0] = accMg[0];
        imuBaselineMg[1] = accMg[1];
        imuBaselineMg[2] = accMg[2];
        imuBaselineValid = 1U;
        imuRecalibrateBaseline = 0U;
    }

    thresholdExceeded = ((SensorService_IsAxisDeltaOverThreshold(accMg[0], imuBaselineMg[0]) != 0U) ||
                         (SensorService_IsAxisDeltaOverThreshold(accMg[1], imuBaselineMg[1]) != 0U) ||
                         (SensorService_IsAxisDeltaOverThreshold(accMg[2], imuBaselineMg[2]) != 0U)) ? 1U : 0U;

    g_sensorData.imuAccMg[0] = accMg[0];
    g_sensorData.imuAccMg[1] = accMg[1];
    g_sensorData.imuAccMg[2] = accMg[2];
    g_sensorData.imuDataValid = 1U;

    if ((thresholdExceeded != 0U) && (g_sensorData.imuAlarmActive == 0U))
    {
        latchAlarm = 1U;
        g_sensorData.imuFallDetected = 1U;
        g_sensorData.imuAlarmActive = 1U;
    }
    osMutexRelease(Group05_SensMtx);

    if (latchAlarm != 0U)
    {
        AppBuzzer_Play(APP_BUZZER_PATTERN_CRITICAL);
        printf("[SH3001] fall alarm ax=%d ay=%d az=%d base=%d,%d,%d threshold=%ldmg\r\n",
               (int)accMg[0],
               (int)accMg[1],
               (int)accMg[2],
               (int)imuBaselineMg[0],
               (int)imuBaselineMg[1],
               (int)imuBaselineMg[2],
               (long)SENSOR_IMU_AXIS_ALARM_THRESHOLD_MG);
    }
}

static void SensorService_UpdateEcg(void)
{
    uint8_t lop = AD8232_ReadLOP();
    uint8_t lon = AD8232_ReadLON();
    uint8_t leadsOff = 0U;
    uint16_t raw = 0;
    uint16_t filtered = 0;
    uint32_t now = HAL_GetTick();

    raw = AD8232_ReadRaw();
    filtered = SensorService_FilterEcg(raw);

    if ((lop != 0U) || (lon != 0U) ||
        (raw <= SENSOR_ECG_LEADS_OFF_LOW_RAW) ||
        (raw >= SENSOR_ECG_LEADS_OFF_HIGH_RAW))
    {
        leadsOff = 1U;
    }

    if ((now - lastEcgLogTick) >= 250U)
    {
        lastEcgLogTick = now;
        printf("[ECG] raw=%u filtered=%u leadsOff=%u lop=%u lon=%u\r\n",
               raw,
               filtered,
               leadsOff,
               lop,
               lon);
    }

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.ecgLeadsOff = leadsOff;
    g_sensorData.ecgRaw = raw;
    g_sensorData.ecgFiltered = filtered;
    osMutexRelease(Group05_SensMtx);
}

static void SensorService_UpdatePressure(void)
{
    uint32_t sum = 0U;
		uint8_t i;

		(void)ReadAdcChannel(ADC_CHANNEL_5);

		for (i = 0U; i < 16U; i++)
		{
				sum += ReadAdcChannel(ADC_CHANNEL_5);
		}

		uint16_t raw = (uint16_t)(sum / 16U);
		
		if (pressureFilterReady == 0U)
		{
				pressureFilteredRaw = (float)raw;
				pressureFilterReady = 1U;
		}
		else
		{
				pressureFilteredRaw = (0.85f * pressureFilteredRaw) + (0.15f * (float)raw);
		}

		raw = (uint16_t)(pressureFilteredRaw + 0.5f);
		
    uint32_t mv = ((uint32_t)raw * SENSOR_VREF_MV) / SENSOR_ADC_MAX_VALUE;
    float bpMmHg = BP_Service_CalcPressureFromAdc(raw);
    uint32_t mmHgTenths = (bpMmHg > 0.0f) ? (uint32_t)((bpMmHg * 10.0f) + 0.5f) : 0U;
    float percent = ((float)mv / (float)SENSOR_VREF_MV) * 100.0f;
    uint32_t now = HAL_GetTick();

    if (pressureStartLogPending != 0U)
    {
        pressureStartLogPending = 0U;
        printf("[MSP20] start raw=%u mv=%lu pressure=%lu.%lu mmHg percent=%lu\r\n",
               raw,
               (unsigned long)mv,
               (unsigned long)(mmHgTenths / 10U),
               (unsigned long)(mmHgTenths % 10U),
               (unsigned long)((mv * 100U) / SENSOR_VREF_MV));
    }

    if ((now - lastPressureLogTick) >= 250U)
    {
        lastPressureLogTick = now;
        printf("[MSP20] raw=%u mv=%lu pressure=%lu.%lu mmHg percent=%lu\r\n",
               raw,
               (unsigned long)mv,
               (unsigned long)(mmHgTenths / 10U),
               (unsigned long)(mmHgTenths % 10U),
               (unsigned long)((mv * 100U) / SENSOR_VREF_MV));
    }

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.pressureRaw = raw;
    g_sensorData.pressureMv = mv;
    g_sensorData.pressureMmHgTenths = mmHgTenths;
    g_sensorData.pressurePercent = percent;
    osMutexRelease(Group05_SensMtx);
}

static void SensorService_UpdatePulse(void)
{
    float data[2] = {0};
    uint32_t ir = 0;
    uint32_t red = 0;
    uint32_t now = HAL_GetTick();
    uint8_t logMode = 0U;
    uint16_t logCount = 0U;
    uint16_t logHeartRate = 0U;
    int logSpo2Tenths = 0;
    uint16_t groupHeartRate = 0U;
    float groupSpo2 = 0.0f;
    uint8_t groupValid = 0U;

    max30102_fifo_read(data);

    ir = (uint32_t)data[0];
    red = (uint32_t)data[1];

    osMutexAcquire(Group05_SensMtx, osWaitForever);
    g_sensorData.ppgIr = ir;
    g_sensorData.ppgRed = red;

    if ((ir > SENSOR_PPG_VALID_THRESHOLD) && (red > SENSOR_PPG_VALID_THRESHOLD))
    {
        g_sensorData.pulseDataValid = 1U;
        irBuffer[ppgIndex] = (float)ir;
        redBuffer[ppgIndex] = (float)red;
        ppgIndex++;
        g_sensorData.pulseValidCount = (uint16_t)(ppgGroupIndex * SENSOR_PPG_GROUP_SAMPLES);
        g_sensorData.pulseProgressPercent = (uint8_t)((g_sensorData.pulseValidCount * 100U) / SENSOR_PPG_TOTAL_SAMPLES);

        if (ppgIndex >= SENSOR_PPG_GROUP_SAMPLES)
        {
            groupHeartRate = max30102_getHeartRate(irBuffer, SENSOR_PPG_GROUP_SAMPLES);
            groupSpo2 = max30102_getSpO2(irBuffer, redBuffer, SENSOR_PPG_GROUP_SAMPLES);
            groupValid = SensorService_IsPulseResultValid(groupHeartRate, groupSpo2);
            ppgIndex = 0;

            if (groupValid != 0U)
            {
                g_sensorData.heartRate = groupHeartRate;
                g_sensorData.spo2 = groupSpo2;
                ppgHeartRateSum += groupHeartRate;
                ppgSpo2Sum += groupSpo2;
                ppgGroupIndex++;
                g_sensorData.pulseValidCount = (uint16_t)(ppgGroupIndex * SENSOR_PPG_GROUP_SAMPLES);
                g_sensorData.pulseProgressPercent = (uint8_t)((g_sensorData.pulseValidCount * 100U) / SENSOR_PPG_TOTAL_SAMPLES);

                if (ppgGroupIndex >= SENSOR_PPG_GROUP_COUNT)
                {
                    g_sensorData.heartRate = (uint16_t)((ppgHeartRateSum + (SENSOR_PPG_GROUP_COUNT / 2U)) / SENSOR_PPG_GROUP_COUNT);
                    g_sensorData.spo2 = ppgSpo2Sum / (float)SENSOR_PPG_GROUP_COUNT;
                    g_sensorData.pulseDone = 1U;
                    g_sensorData.pulseRunning = 0U;
                    g_sensorData.pulseValidCount = SENSOR_PPG_TOTAL_SAMPLES;
                    g_sensorData.pulseProgressPercent = 100U;
                    logMode = 1U;
                    logHeartRate = g_sensorData.heartRate;
                    logSpo2Tenths = (int)(g_sensorData.spo2 * 10.0f);
                    ppgGroupIndex = 0;
                    ppgHeartRateSum = 0;
                    ppgSpo2Sum = 0.0f;
                }
                else if ((now - lastPulseLogTick) >= 250U)
                {
                    lastPulseLogTick = now;
                    logMode = 2U;
                    logCount = g_sensorData.pulseValidCount;
                    logHeartRate = g_sensorData.heartRate;
                    logSpo2Tenths = (int)(g_sensorData.spo2 * 10.0f);
                }
            }
            else
            {
                g_sensorData.pulseValidCount = (uint16_t)(ppgGroupIndex * SENSOR_PPG_GROUP_SAMPLES);
                g_sensorData.pulseProgressPercent = (uint8_t)((g_sensorData.pulseValidCount * 100U) / SENSOR_PPG_TOTAL_SAMPLES);
                lastPulseLogTick = now;
                logMode = 4U;
                logCount = g_sensorData.pulseValidCount;
                logHeartRate = groupHeartRate;
                logSpo2Tenths = (int)(groupSpo2 * 10.0f);
            }
        }
        else if ((now - lastPulseLogTick) >= 250U)
        {
            lastPulseLogTick = now;
            logMode = 2U;
            logCount = g_sensorData.pulseValidCount;
            logHeartRate = g_sensorData.heartRate;
            logSpo2Tenths = (int)(g_sensorData.spo2 * 10.0f);
        }
    }
    else
    {
        g_sensorData.pulseDataValid = 0U;
        ppgIndex = 0U;
        g_sensorData.pulseValidCount = (uint16_t)(ppgGroupIndex * SENSOR_PPG_GROUP_SAMPLES);
        g_sensorData.pulseProgressPercent = (uint8_t)((g_sensorData.pulseValidCount * 100U) / SENSOR_PPG_TOTAL_SAMPLES);
        if ((now - lastPulseLogTick) >= 500U)
        {
            lastPulseLogTick = now;
            logMode = 3U;
        }
    }

    osMutexRelease(Group05_SensMtx);

    if (logMode == 1U)
    {
        printf("[MAX30102] done spo2=%d.%d hr=%u red=%lu ir=%lu\r\n",
               logSpo2Tenths / 10,
               logSpo2Tenths % 10,
               logHeartRate,
               (unsigned long)red,
               (unsigned long)ir);
    }
    else if (logMode == 2U)
    {
        printf("[MAX30102] sample=%u/%u spo2=%d.%d hr=%u red=%lu ir=%lu\r\n",
               logCount,
               (unsigned int)SENSOR_PPG_TOTAL_SAMPLES,
               logSpo2Tenths / 10,
               logSpo2Tenths % 10,
               logHeartRate,
               (unsigned long)red,
               (unsigned long)ir);
    }
    else if (logMode == 3U)
    {
        printf("[MAX30102] no-finger spo2=0.0 hr=0 red=%lu ir=%lu\r\n",
               (unsigned long)red,
               (unsigned long)ir);
    }
    else if (logMode == 4U)
    {
        printf("[MAX30102] reject sample=%u/%u spo2=%d.%d hr=%u red=%lu ir=%lu\r\n",
               logCount,
               (unsigned int)SENSOR_PPG_TOTAL_SAMPLES,
               logSpo2Tenths / 10,
               logSpo2Tenths % 10,
               logHeartRate,
               (unsigned long)red,
               (unsigned long)ir);
    }
}

void SensorService_Task(void *argument)
{
    uint32_t lastEcg = 0;
    uint32_t lastPressure = 0;
    uint32_t lastPulse = 0;
		uint32_t lastChipTemp = 0;
    uint32_t lastImu = 0;
    uint32_t lastImuInit = 0;

    SensorService_Init();

    for (;;)
    {
        uint32_t now = HAL_GetTick();
        SensorData_t snapshot;

        SensorService_GetData(&snapshot);

        if ((now - lastChipTemp) >= SENSOR_TEMP_PERIOD_MS)
        {
            lastChipTemp = now;
            SensorService_UpdateChipTemp();
        }

        if (snapshot.imuRunning != 0U)
        {
            if ((snapshot.imuReady == 0U) &&
                ((lastImuInit == 0U) || ((now - lastImuInit) >= SENSOR_IMU_INIT_RETRY_PERIOD_MS)))
            {
                lastImuInit = now;
                SensorService_TryInitImu();
            }

            if ((snapshot.imuReady != 0U) && ((now - lastImu) >= SENSOR_IMU_SAMPLE_PERIOD_MS))
            {
                lastImu = now;
                SensorService_UpdateImu();
            }
        }

        if ((snapshot.ecgRunning != 0U) && ((now - lastEcg) >= SENSOR_ECG_SAMPLE_PERIOD_MS))
        {
            lastEcg = now;
            SensorService_UpdateEcg();
        }

        if ((snapshot.pressureRunning != 0U) && ((now - lastPressure) >= SENSOR_PRESSURE_SAMPLE_PERIOD_MS))
        {
            lastPressure = now;
            SensorService_UpdatePressure();
        }

        if ((snapshot.pulseRunning != 0U) && ((now - lastPulse) >= SENSOR_PPG_SAMPLE_PERIOD_MS))
        {
            lastPulse = now;
            SensorService_UpdatePulse();
        }

        osDelay(1);
    }
}
void StartTaskSensorService(void *argument)
{
    SensorService_Task(argument);
}

void StartTask04(void *argument)
{
    SensorService_Task(argument);
}
