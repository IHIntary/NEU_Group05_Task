# Group05 Project Hardware Summary

本文档记录当前 Group05 工程的传感器接口、输入输出、按键、LCD/TouchGFX 屏幕接口和关键 CubeMX 配置，方便后续整合成大项目时快速查线和查代码。

## 1. MCU 与工程状态

| 项目 | 当前配置 |
| --- | --- |
| MCU | STM32F429IGT6 |
| 封装 | LQFP176 |
| 主频 | 180 MHz |
| 工具链 | MDK-ARM / Keil uVision |
| HAL 工程 | STM32CubeMX 生成，保留 USER CODE |
| RTOS | FreeRTOS CMSIS-RTOS V2 |
| UI 框架 | TouchGFX 4.22.0 |
| 屏幕分辨率 | 1024 x 600 |
| 像素格式 | RGB565 |
| Framebuffer | 0xC0000000，外部 SDRAM |

当前工程已确认 Keil 编译通过：

```text
"Group05\Group05.axf" - 0 Error(s), 10 Warning(s).
```

剩余 warning 主要是旧的 `app_ui.c` / FreeRTOS 传感器任务函数未被当前 TouchGFX 流程使用。

## 2. 传感器总览

| 模块 | 功能 | MCU 接口 | 输入到 MCU | 输出/显示内容 | 主要代码 |
| --- | --- | --- | --- | --- | --- |
| MAX30102 | 心率 / 血氧 / PPG | I2C2 + INT | 红光/红外 PPG FIFO 数据，中断信号 | IR、RED、Heart Rate、SpO2、传感器 ACK 状态 | `Drivers/MAX30102/` |
| AD8232 | ECG 心电 | ADC1_IN11 + GPIO | ECG 模拟电压、LO+、LO- 脱落检测 | RAW ADC、NORMAL/LEADS OFF、ECG 波形 | `Core/Src/ad8232.c` |
| MSP20 | 压力 | ADC1_IN5 | 模拟电压 | RAW ADC、电压、压力百分比 | `Core/Src/app_ui.c` 中 `ReadAdcChannel(ADC_CHANNEL_5)` |
| GT9XXX | 电容触摸 | 软件 I2C + RST + INT | 触摸坐标 | TouchGFX 点击/滑动输入 | `Drivers/TOUCH/` |

## 3. MAX30102 脉搏血氧模块

### 3.1 接口与引脚

| MAX30102 引脚 | MCU 引脚 | CubeMX Label | 配置 | 说明 |
| --- | --- | --- | --- | --- |
| SCL | PH4 | I2C2_SCL | I2C2 | 硬件 I2C 时钟 |
| SDA | PH5 | I2C2_SDA | I2C2 | 硬件 I2C 数据 |
| INT | PB3 | MAX30102_INT | GPIO_EXTI3，Falling，Pull-up | FIFO 数据就绪中断 |
| VIN / VCC | 3.3V | - | 电源 | 模块供电 |
| GND | GND | - | 地 | 共地 |

### 3.2 I2C 地址与数据

| 项目 | 值 |
| --- | --- |
| 写地址 | `0xAE` |
| 读地址 | `0xAF` |
| HAL I2C | `hi2c2` |
| 缓存长度 | `CACHE_NUMS = 150` |
| 有效接触阈值 | `PPG_DATA_THRESHOLD = 100000` |

### 3.3 软件输出

MAX30102 驱动输出：

| 输出 | 类型 | 说明 |
| --- | --- | --- |
| IR | `float` / `uint32_t` | 红外原始 PPG 数据 |
| RED | `float` / `uint32_t` | 红光原始 PPG 数据 |
| HeartRate | `uint16_t` | 计算后的心率 BPM |
| SpO2 | `float` | 血氧百分比 |
| `max30102_int_flag` | `volatile uint8_t` | PB3 EXTI 置位的数据就绪标志 |

主要函数：

```c
void max30102_init(void);
void max30102_fifo_read(float *data);
uint16_t max30102_getHeartRate(float *input_data, uint16_t cache_nums);
float max30102_getSpO2(float *ir_input_data, float *red_input_data, uint16_t cache_nums);
uint8_t MAX30102_Get_DATA(uint16_t *HeartRate, float *SpO2);
```

## 4. AD8232 ECG 模块

### 4.1 接口与引脚

| AD8232 引脚 | MCU 引脚 | CubeMX Label | 配置 | 说明 |
| --- | --- | --- | --- | --- |
| OUTPUT | PC1 | ADC1_IN11 | ADC1 Channel 11，Analog | ECG 模拟输出 |
| LO+ | PC4 | AD8232_LOP | GPIO Input，Pull-down | 电极脱落检测，高电平表示脱落 |
| LO- | PC5 | AD8232_LON | GPIO Input，Pull-down | 电极脱落检测，高电平表示脱落 |
| 3.3V | 3.3V | - | 电源 | 模块供电 |
| GND | GND | - | 地 | 共地 |

### 4.2 软件行为

| 状态 | 判断条件 | UI 行为 |
| --- | --- | --- |
| NORMAL | PC4 = 0 且 PC5 = 0 | 显示 ECG 波形 |
| LEADS OFF | PC4 = 1 或 PC5 = 1 | 显示 LEADS OFF，不继续绘制波形 |

主要函数：

```c
uint16_t AD8232_ReadRaw(void);
float AD8232_ReadVoltage(void);
uint8_t AD8232_IsLeadsOff(void);
uint8_t AD8232_ReadLOP(void);
uint8_t AD8232_ReadLON(void);
uint32_t AD8232_GetLastAdcError(void);
```

### 4.3 ECG 显示和滤波

旧 LCD UI 中 ECG 波形区域：

| 项目 | 值 |
| --- | --- |
| X 起点 | `GRAPH_X = 40` |
| Y 起点 | `GRAPH_Y = 210` |
| 宽度 | `GRAPH_W = 944` |
| 高度 | `GRAPH_H = 310` |

滤波逻辑在 `Core/Src/app_ui.c`：

```c
static void EcgFilterReset(uint16_t raw);
static int32_t EcgFilterSample(uint16_t raw);
static uint16_t MapFilteredToGraphY(int32_t filtered);
```

说明：

- 使用慢速基线跟踪去除直流漂移。
- 使用简单低通平滑减少毛刺。
- 使用单步限幅避免折线突然跳变。

## 5. MSP20 压力模块

### 5.1 接口与引脚

| MSP20 引脚 | MCU 引脚 | CubeMX Signal | 配置 | 说明 |
| --- | --- | --- | --- | --- |
| Analog OUT | PA5 | ADC1_IN5 | ADC Analog | 压力模拟电压输入 |
| VCC | 3.3V / 传感器要求电源 | - | 电源 | 按实际模块电压要求接 |
| GND | GND | - | 地 | 共地 |

### 5.2 当前软件计算

当前压力读取使用：

```c
#define MSP20_ADC_CHANNEL ADC_CHANNEL_5
```

当前显示计算：

```c
uint16_t raw = ReadAdcChannel(ADC_CHANNEL_5);
uint32_t mv = ((uint32_t)raw * 3300U) / 4095U;
float pressure = ((float)mv / 3300.0f) * 100.0f;
```

输出内容：

| 输出 | 说明 |
| --- | --- |
| RAW ADC | 12 位 ADC 原始值，0-4095 |
| Voltage | 换算电压，单位 V |
| Pressure | 当前按 `%FS` 百分比显示，尚未做真实标定曲线 |

注意：PA5 已在 CubeMX 中设置为 ADC 输入，但 ADC1 Regular Conversion 当前默认主要是 Channel 11。代码里通过 `HAL_ADC_ConfigChannel()` 动态切换 Channel 5/11。

## 6. 按键接口

| 按键 | MCU 引脚 | CubeMX Label | GPIO 配置 | 有效电平 | 当前功能 |
| --- | --- | --- | --- | --- | --- |
| KEY0 | PH3 | KEY0 | GPIO Input，Pull-up | 低电平按下 | 进入 MAX30102 脉搏界面 |
| KEY1 | PH2 | KEY1 | GPIO Input，Pull-up | 低电平按下 | 进入 AD8232 ECG 界面 |
| KEY2 | PC13 | KEY2 | GPIO Input，代码中补 Pull-up | 低电平按下 | 进入 MSP20 压力界面 |
| WKUP | PA0 | WKUP | GPIO Input，Pull-down | 高电平按下 | 显示 WKUP 提示并返回主页 |

按键扫描函数：

```c
uint8_t key_scan(uint8_t mode);
```

返回值：

```c
#define KEY0_PRES  1U
#define KEY1_PRES  2U
#define KEY2_PRES  3U
#define WKUP_PRES  4U
```

## 7. LED 与基础调试输出

| 功能 | MCU 引脚 | Label | 说明 |
| --- | --- | --- | --- |
| LED0 | PB1 | LED0 | 当前代码中每 500 ms 翻转 |
| LED1 | PB0 | LED1 | 旧任务中使用 |
| USART1_TX | PA9 | USART1_TX | printf 调试串口 TX |
| USART1_RX | PA10 | USART1_RX | printf 调试串口 RX |

注意：PB0/PB1 在当前 `.ioc` 中仍显示为部分 LTDC RGB 复用信号，但代码中有 `LTDC_ForceApolloPins()` / `ReInitBoardPins()` 类补丁把板载 LED 用法救回来。后续如果要减少手写补丁，建议在 CubeMX 中彻底修正 LTDC 引脚后再把 PB0/PB1 作为 LED 输出。

## 8. LCD RGB 屏幕接口

### 8.1 显示参数

| 项目 | 当前值 |
| --- | --- |
| 屏幕类型 | RGB LCD |
| 控制器 | STM32 LTDC |
| 分辨率 | 1024 x 600 |
| 像素格式 | RGB565 |
| Framebuffer | `0xC0000000` |
| Framebuffer 存储 | 外部 SDRAM |
| 背光 | PB5 / LCD_BL |

`Core/Inc/ltdc.h` 中定义：

```c
#define LCD_WIDTH        1024U
#define LCD_HEIGHT       600U
#define LCD_FRAME_BUFFER 0xC0000000U
```

### 8.2 LTDC 主要信号

| 信号 | MCU 引脚 |
| --- | --- |
| LTDC_CLK | PG7 |
| LTDC_DE | PF10 |
| LTDC_HSYNC | PI10 |
| LTDC_VSYNC | PI9 |
| LTDC_R3 | 当前 `.ioc`: PB0；Apollo 修正目标通常应避免占用 LED |
| LTDC_R4 | PH10 |
| LTDC_R5 | PH11 |
| LTDC_R6 | 当前 `.ioc`: PB1；Apollo 修正目标通常应避免占用 LED |
| LTDC_R7 | PG6 |
| LTDC_G2 | 当前 `.ioc`: PA6 |
| LTDC_G3 | PH14 |
| LTDC_G4 | 当前 `.ioc`: PB10 |
| LTDC_G5 | 当前 `.ioc`: PB11 |
| LTDC_G6 | 当前 `.ioc`: PC7 |
| LTDC_G7 | PI2 |
| LTDC_B3 | PG11 |
| LTDC_B4 | 当前 `.ioc`: PG12 |
| LTDC_B5 | 当前 `.ioc`: PA3 |
| LTDC_B6 | 当前 `.ioc`: PB8 |
| LTDC_B7 | 当前 `.ioc`: PB9 |

注意：当前工程能显示，部分原因是 `ltdc.c` 中保留了 Apollo LCD 的强制修正代码。后续做大项目时，建议把 LTDC 引脚和 timing 完全在 CubeMX 中修正，减少手写兜底。

### 8.3 背光

| 功能 | MCU 引脚 | Label | 有效电平 |
| --- | --- | --- | --- |
| LCD 背光 | PB5 | LCD_BL | 高电平点亮 |

代码中通常使用：

```c
HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);
```

## 9. 触摸屏 GT9XXX 接口

### 9.1 引脚

| GT9XXX / 触摸屏信号 | MCU 引脚 | CubeMX Label | 配置 | 说明 |
| --- | --- | --- | --- | --- |
| CT_IIC_SCL | PH6 | CT_IIC_SCL | GPIO Output OD，Pull-up | 软件 I2C SCL |
| CT_IIC_SDA | PI3 | CT_IIC_SDA | GPIO Output OD，Pull-up | 软件 I2C SDA |
| RST | PI8 | GT9XXX_RST | GPIO Output PP，Pull-up | 触摸芯片复位 |
| INT | PH7 | GT9XXX_INT | GPIO Input | 触摸中断/地址选择相关 |

触摸驱动不是使用硬件 I2C2，而是 `Drivers/TOUCH/ctiic.c` 软件 I2C。

### 9.2 主要函数

```c
uint8_t gt9xxx_init(void);
uint8_t gt9xxx_scan(uint8_t mode, int32_t* x, int32_t* y);
```

TouchGFX 触摸输入桥接在：

```text
TouchGFX/target/STM32TouchController.cpp
```

触摸初始化任务在：

```text
Core/Src/freertos.c -> taskTouchInit()
```

## 10. SDRAM / FMC 接口

LCD framebuffer 放在外部 SDRAM，因此 FMC 是屏幕显示的基础。

### 10.1 SDRAM 参数

| 参数 | 当前值 |
| --- | --- |
| Bank | FMC_SDRAM_BANK1 |
| Data Width | 16 bit |
| Row Bits | 13 |
| Column Bits | 代码强制修正为 9 |
| Internal Banks | 4 banks |
| CAS Latency | 3 |
| SD Clock Period | HCLK / 2 |
| Read Burst | Enable |
| Read Pipe Delay | 1 |
| Refresh Rate | 1386 |

当前 `fmc.c` 中有两个重要补丁：

```c
static void SDRAM_ForceApolloConfig(void);
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram);
```

说明：CubeMX 当前 `.ioc` 里 SDRAM column bits 仍容易被生成成 8 bits，代码里通过 `SDRAM_ForceApolloConfig()` 修正为 9 bits。后续建议在 CubeMX FMC SDRAM 配置中改成 9-bit column。

## 11. ADC1 使用情况

| ADC 通道 | MCU 引脚 | 用途 | 当前读取方式 |
| --- | --- | --- | --- |
| ADC1_IN11 | PC1 | AD8232 ECG OUTPUT | `AD8232_ReadRaw()` |
| ADC1_IN5 | PA5 | MSP20 压力模拟输入 | `ReadAdcChannel(ADC_CHANNEL_5)` |

ADC 配置：

| 项目 | 值 |
| --- | --- |
| 分辨率 | 12 bit |
| 参考电压 | 3.3V |
| 数值范围 | 0-4095 |
| 采样时间 | `ADC_SAMPLETIME_144CYCLES` |
| 触发 | Software Start |

注意：多个传感器共用 ADC1，代码读取前会动态调用 `HAL_ADC_ConfigChannel()` 切换通道。

## 12. FreeRTOS / TouchGFX 任务

当前 `.ioc` 中任务：

| 任务名 | 入口函数 | 优先级 | 栈大小 | 作用 |
| --- | --- | --- | --- | --- |
| Group05_DefTask | `StartDefaultTask` | Normal | 128 words | 周期调用 `TouchGFX_SignalVSync()` |
| Group05_TGFX | `TouchGFX_Task` | AboveNormal | 1024 words | 调用 `MX_TouchGFX_Process()`，运行 UI |
| Group05_TInit | `taskTouchInit` | BelowNormal | 512 words | 延时后初始化 GT9XXX 触摸 |

当前已避免 `TouchGFX_Task` 重名冲突：

- `TouchGFX/App/app_touchgfx.c` 内部已经定义了 `TouchGFX_Task()`
- FreeRTOS 任务入口改为 `TouchGFX_RunTask()`

## 13. 后续大项目集成建议

### 13.1 建议保留的接口分层

建议后续大项目按下面分层：

```text
Drivers/
  MAX30102/
  TOUCH/
Core/
  Src/ad8232.c
  Src/app_sensor_service.c      // 建议新增：统一采集传感器
TouchGFX/
  gui/                          // 页面逻辑
  target/                       // HAL / 触摸桥接
```

### 13.2 建议统一的数据结构

后续可以把传感器数据统一成一个状态结构，供 TouchGFX Model 或 Presenter 读取：

```c
typedef struct
{
  uint16_t ecgRaw;
  uint8_t ecgLeadsOff;
  uint16_t pressureRaw;
  uint32_t pressureMv;
  float pressurePercent;
  uint32_t ppgIr;
  uint32_t ppgRed;
  uint16_t heartRate;
  float spo2;
  uint8_t max30102Ready;
} SensorData_t;
```

### 13.3 CubeMX 中建议继续修正

| 项目 | 当前状态 | 建议 |
| --- | --- | --- |
| FMC SDRAM Column Bits | 代码强制修正为 9 | CubeMX 中改为 9 bits |
| LTDC 引脚 | 部分仍依赖代码修正 | CubeMX 中按 Apollo LCD 正确引脚彻底修正 |
| ADC1 Channel 5 | PA5 已为 Analog，但 Regular 主要是 IN11 | 若想少手写，可把 IN5 也加入 ADC Regular 配置 |
| DMA2D | 当前未作为核心依赖 | TouchGFX 项目建议启用 DMA2D/ChromART |
| PB0/PB1 LED | 代码中补回 LED | CubeMX 中避免与 LTDC R3/R6 冲突 |

## 14. 快速接线清单

### 14.1 传感器

| 模块 | 信号 | MCU |
| --- | --- | --- |
| MAX30102 | SCL | PH4 / I2C2_SCL |
| MAX30102 | SDA | PH5 / I2C2_SDA |
| MAX30102 | INT | PB3 / EXTI3 |
| AD8232 | OUTPUT | PC1 / ADC1_IN11 |
| AD8232 | LO+ | PC4 / GPIO Input |
| AD8232 | LO- | PC5 / GPIO Input |
| MSP20 | Analog OUT | PA5 / ADC1_IN5 |
| GT9XXX Touch | SCL | PH6 / GPIO OD |
| GT9XXX Touch | SDA | PI3 / GPIO OD |
| GT9XXX Touch | RST | PI8 / GPIO Output |
| GT9XXX Touch | INT | PH7 / GPIO Input |

### 14.2 按键

| 按键 | MCU | 有效电平 |
| --- | --- | --- |
| KEY0 | PH3 | Low |
| KEY1 | PH2 | Low |
| KEY2 | PC13 | Low |
| WKUP | PA0 | High |

### 14.3 屏幕

| 功能 | MCU |
| --- | --- |
| LTDC RGB 数据/同步 | LTDC 外设多引脚 |
| SDRAM Framebuffer | FMC SDRAM，0xC0000000 |
| 背光 | PB5 / LCD_BL，高电平亮 |
| 触摸 | GT9XXX，软件 I2C PH6/PI3 |
