# Group05 硬件与调试说明

本文档基于当前工程文件整理，重点参考 `NEU_Group05_Task.ioc`、`Core/`、`Drivers/`、`TouchGFX/` 中的实际代码。用于后续接线核对、传感器调试、CubeMX 重新生成代码、TouchGFX 页面增改和硬件扩展。

## 1. 工程概览

| 项目 | 当前配置 |
| --- | --- |
| 工程名 | NEU_Group05_Task |
| MCU | STM32F429IGT6 |
| 封装 | LQFP176 |
| 工具链 | MDK-ARM V5.32 |
| CubeMX 文件 | `NEU_Group05_Task.ioc` |
| TouchGFX 文件 | `TouchGFX/NEU_Group05_Task.touchgfx` |
| UI 框架 | TouchGFX 4.22.0 |
| RTOS | FreeRTOS CMSIS-RTOS V2 |
| 屏幕分辨率 | 1024 x 600 |
| 像素格式 | RGB565 |
| Framebuffer | `0xC0000000`，外部 SDRAM |
| 串口调试 | USART1，115200 8N1 |

当前工程主入口为 `Core/Src/main.c`。初始化顺序大致为：

1. HAL、系统时钟、delay。
2. GPIO、I2C2、USART1、ADC1、TIM8、FMC。
3. SDRAM framebuffer 自检并清屏。
4. LTDC、CRC、DMA2D。
5. TouchGFX 初始化。
6. FreeRTOS 创建任务并启动调度。

## 2. 传感器与外设总览

| 模块 | 功能 | 接口 | 主要引脚 | 主要代码 |
| --- | --- | --- | --- | --- |
| MAX30102 | 心率、血氧、PPG 原始数据 | I2C2 + EXTI | PH4、PH5、PB3 | `Drivers/MAX30102/` |
| AD8232 | ECG 心电 | ADC1 + GPIO | PC1、PC4、PC5 | `Core/Src/ad8232.c` |
| MSP20 | 压力模拟量 | ADC1 | PA5 | `Core/Src/app_sensor_service.c` |
| GT9XXX | 电容触摸 | 软件 I2C + RST/INT | PH6、PI3、PI8、PH7 | `Drivers/TOUCH/` |
| RGB LCD | 1024x600 显示 | LTDC + FMC SDRAM | LTDC 多引脚、PB5 背光 | `Core/Src/ltdc.c` |
| 蜂鸣器 | 点击/操作提示 | I2C2 | PH4、PH5 | `Core/Src/buzzer.c` |
| 按键 | 本地输入 | GPIO | PH3、PH2、PC13、PA0 | `Core/Src/key.c` |
| LED | 状态显示 | GPIO | PB1、PB0 | `TouchGFX/gui/src/model/Model.cpp` |

## 3. MAX30102 心率血氧模块

| 信号 | MCU 引脚 | CubeMX 配置 | 代码/说明 |
| --- | --- | --- | --- |
| SCL | PH4 | I2C2_SCL | 硬件 I2C2 |
| SDA | PH5 | I2C2_SDA | 硬件 I2C2 |
| INT | PB3 | GPIO_EXTI3，下降沿，上拉 | `HAL_GPIO_EXTI_Callback()` 中置 `max30102_int_flag` |
| VCC | 3.3V | - | 按模块要求供电 |
| GND | GND | - | 共地 |

I2C2 配置：

| 项目 | 值 |
| --- | --- |
| I2C 实例 | `hi2c2` |
| 速度 | 100 kHz |
| MAX30102 写地址 | `0xAE` |
| MAX30102 读地址 | `0xAF` |
| 有效接触阈值 | `100000` |
| 单组缓存 | `150` 点 |
| 当前 TouchGFX 统计 | 3 组，每组 150 点，共 450 点 |

当前软件行为：

- `SensorService_Init()` 中调用 `max30102_init()`。
- TouchGFX Pulse 页面调用 `Model::setPulseRunning(1)` 后，`SensorService_Task()` 每 10 ms 采样一次。
- `SensorService_UpdatePulse()` 读取 FIFO，更新 IR、RED、进度、心率和血氧。
- 原驱动里也保留了基于 `max30102_int_flag` 的 `MAX30102_Get_DATA()`，但当前 TouchGFX 主流程主要用定时采样方式读取 FIFO。

调试建议：

- 先用串口看 `[MAX30102] no-finger`、`sample=x/450`、`done` 日志。
- 如果一直无数据，先检查 PH4/PH5 上拉、模块供电、I2C 地址和 PB3 中断线。
- 蜂鸣器也挂在 I2C2 上，调试时注意 I2C2 总线是否被某个设备拉死。

## 4. AD8232 ECG 心电模块

| 信号 | MCU 引脚 | CubeMX 配置 | 有效状态 |
| --- | --- | --- | --- |
| OUTPUT | PC1 | ADC1_IN11，Analog | 12 bit ADC，0-4095 |
| LO+ | PC4 | GPIO Input，Pull-down | 高电平表示导联脱落 |
| LO- | PC5 | GPIO Input，Pull-down | 高电平表示导联脱落 |
| VCC | 3.3V | - | 按模块要求供电 |
| GND | GND | - | 共地 |

ADC1 配置：

| 项目 | 值 |
| --- | --- |
| 分辨率 | 12 bit |
| 参考电压换算 | 3.3 V |
| 默认 Regular Channel | ADC_CHANNEL_11 |
| 采样时间 | ADC_SAMPLETIME_144CYCLES |
| 触发方式 | Software Start |
| DMA | 未启用 |

当前软件行为：

- `AD8232_ReadRaw()` 每次读取前强制配置 ADC1 Channel 11。
- `SensorService_UpdateEcg()` 每 5 ms 采样一次。
- 判断导联脱落条件：
  - PC4/PC5 任一为高。
  - ADC 原始值低于 `50` 或高于 `4045`。
- ECG 滤波在 `SensorService_FilterEcg()` 中完成：
  - 高通去基线漂移，`alpha = 0.98`。
  - 再做 5 点移动平均。
  - 输出 `ecgFiltered` 给 TouchGFX 动态曲线。

调试建议：

- 串口关注 `[ECG] raw=... filtered=... leadsOff=... lop=... lon=...`。
- 如果 UI 一直显示导联脱落，先测 PC4/PC5 电平，再看 PC1 ADC 是否贴近 0 或 4095。
- ADC1 被 AD8232 和 MSP20 共用，所有读取都通过 `HAL_ADC_ConfigChannel()` 动态切换通道。

## 5. MSP20 压力模块

| 信号 | MCU 引脚 | CubeMX 配置 | 说明 |
| --- | --- | --- | --- |
| Analog OUT | PA5 | ADC1_IN5，Analog | 压力模拟电压输入 |
| VCC | 按模块要求 | - | 注意不同 MSP20 模块供电要求可能不同 |
| GND | GND | - | 共地 |

当前软件行为：

- `SensorService_UpdatePressure()` 每 20 ms 读取 `ADC_CHANNEL_5`。
- 原始 ADC 换算：
  - `mv = raw * 3300 / 4095`
  - `percent = mv / 3300 * 100`
  - `mmHgTenths = (mv * 10 / 33) * 760` 的等价整数计算
- 串口日志格式：`[MSP20] raw=... mv=... mmHg=... percent=...`

注意：

- 当前 `SENSOR_PRESSURE_ATM_MV = 33` 表示把 33 mV 当作 1 atm 的参考点，这更像临时标定参数。
- 如果要做真实压力测量，需要根据实际 MSP20 放大电路和传感器量程重新标定。

## 6. GT9XXX 电容触摸

| 信号 | MCU 引脚 | CubeMX 配置 | 说明 |
| --- | --- | --- | --- |
| CT_IIC_SCL | PH6 | GPIO Output OD，Pull-up | 软件 I2C SCL |
| CT_IIC_SDA | PI3 | GPIO Output OD，Pull-up | 软件 I2C SDA |
| RST | PI8 | GPIO Output PP，Pull-up | 触摸芯片复位 |
| INT | PH7 | GPIO Input | 初始化时会短暂改成输出用于地址选择 |

当前软件行为：

- 触摸不使用硬件 I2C2，而是 `Drivers/TOUCH/ctiic.c` 软件 I2C。
- `Group05_TInit` 任务延时 300 ms 后调用 `gt9xxx_init()`。
- `gt9xxx_init()` 会尝试多组地址组合：
  - `0x28 / 0x29`
  - `0xBA / 0xBB`
- TouchGFX 触摸桥接在 `TouchGFX/target/STM32TouchController.cpp`。
- `TouchGFX_Touch_SetReady(ret)` 中，`ret == 0` 表示初始化成功，TouchGFX 才开始采样。

调试建议：

- 串口会打印 `GT9 probe`、`GT9 PID raw`、`CTP ID`。
- 如果触摸无效，先确认 PI8 复位、PH7 INT、PH6/PI3 软件 I2C 波形。
- 坐标合法范围在驱动中限制为 `x <= 1024`、`y <= 600`。

## 7. 按键

| 按键 | MCU 引脚 | CubeMX/代码配置 | 有效电平 | 返回值 |
| --- | --- | --- | --- | --- |
| KEY0 | PH3 | GPIO Input，Pull-up | 低电平按下 | `KEY0_PRES = 1` |
| KEY1 | PH2 | GPIO Input，Pull-up | 低电平按下 | `KEY1_PRES = 2` |
| KEY2 | PC13 | GPIO Input；`gpio.c` 当前为 No Pull | 低电平按下 | `KEY2_PRES = 3` |
| WKUP | PA0 | GPIO Input，Pull-down | 高电平按下 | `WKUP_PRES = 4` |

当前软件行为：

- `key_scan()` 做 10 ms 消抖。
- TouchGFX 当前主要用触摸按钮交互。
- `Model::handleKey0()` 会直接扫描 KEY0，按下后切换 LED0。
- `freertos.c` 中仍保留一些旧演示任务函数，例如 LED、KEY、压力打印、MAX30102 打印，但当前 `MX_FREERTOS_Init()` 没创建这些旧任务。

## 8. LED 与蜂鸣器

### LED

| 名称 | MCU 引脚 | 当前代码定义 | 有效电平 |
| --- | --- | --- | --- |
| LED0 | PB1 | `LED0_Pin` | 低电平亮 |
| LED1 | PB0 | `LED1_Pin` | 低电平亮 |

重要注意：

- `NEU_Group05_Task.ioc` 中 PB0/PB1 仍显示为 LTDC_R3/LTDC_R6。
- `Core/Inc/main.h` 手写定义 PB0/PB1 为 LED。
- `LTDC_ForceApolloPins()` 最后会把 PB0/PB1 重新初始化成 GPIO 输出，并置高关闭 LED。
- 如果后续彻底整理 CubeMX，建议把 LCD RGB 引脚和 LED 引脚冲突一次性修正，避免重新生成代码后出问题。

### 蜂鸣器

| 项目 | 值 |
| --- | --- |
| 接口 | I2C2 |
| I2C 地址 | `0x40` |
| 打开蜂鸣 | 写 `0x00` |
| 关闭蜂鸣 | 写 `0x01` |
| 代码 | `Core/Src/buzzer.c` |

`Model::beep(durationMs)` 会调用 `buzzer(durationMs)`，页面 start/end/reset 操作通常会响 60 ms。

## 9. RGB LCD、LTDC 与 TouchGFX

| 项目 | 值 |
| --- | --- |
| 分辨率 | 1024 x 600 |
| 色彩格式 | RGB565 |
| Framebuffer | `0xC0000000` |
| 背光 | PB5 / LCD_BL |
| 背光有效电平 | 高电平点亮 |
| TouchGFX 色深 | 16 bpp |
| 启动页 | HomeScreen |

CubeMX 中的 LTDC 信号：

| 信号 | MCU 引脚 |
| --- | --- |
| LTDC_CLK | PG7 |
| LTDC_DE | PF10 |
| LTDC_HSYNC | PI10 |
| LTDC_VSYNC | PI9 |
| LTDC_R3 | PB0 |
| LTDC_R4 | PH10 |
| LTDC_R5 | PH11 |
| LTDC_R6 | PB1 |
| LTDC_R7 | PG6 |
| LTDC_G2 | PA6 |
| LTDC_G3 | PH14 |
| LTDC_G4 | PB10 |
| LTDC_G5 | PB11 |
| LTDC_G6 | PC7 |
| LTDC_G7 | PI2 |
| LTDC_B3 | PG11 |
| LTDC_B4 | PG12 |
| LTDC_B5 | PA3 |
| LTDC_B6 | PB8 |
| LTDC_B7 | PB9 |

代码中的关键修正：

- `Core/Src/ltdc.c` 中 `LTDC_ForceApolloConfig()` 会重新设置 1024x600 Apollo 屏时序：
  - HSYNC = 20 - 1
  - HBP = 140
  - HFP = 160
  - VSYNC = 3 - 1
  - VBP = 20
  - VFP = 12
- `LTDC_ForceApolloPins()` 会重新配置一批 Apollo LCD 相关 GPIO，并把 PB0/PB1 改回 LED。
- `Core/Src/main.c` 在启动 TouchGFX 前会做 SDRAM framebuffer 自检。

调试建议：

- 黑屏时先看 PB5 背光是否为高。
- 再看 SDRAM 自检是否进入 `Error_Handler()`。
- 再看 LTDC 时钟和 RGB 同步信号。
- 如果从 CubeMX 重新生成代码，务必检查 `ltdc.c` 的 USER CODE 补丁是否还在。

## 10. SDRAM / FMC

Framebuffer 位于外部 SDRAM，因此 FMC 配置直接影响 LCD 显示。

| 项目 | 当前值 |
| --- | --- |
| SDRAM Bank | FMC_SDRAM_BANK1 |
| Data Width | 16 bit |
| Row Bits | 13 |
| Column Bits | 代码强制为 9 bit |
| Internal Banks | 4 banks |
| CAS Latency | 3 |
| SD Clock | HCLK / 2 |
| Read Burst | Enable |
| Read Pipe Delay | 1 |
| Refresh Rate | 1386 |

FMC SDRAM 引脚：

| 类别 | MCU 引脚 |
| --- | --- |
| 地址线 | PF0-PF5, PF12-PF15, PG0-PG2 |
| Bank 地址 | PG4, PG5 |
| 数据线 | PD14, PD15, PD0, PD1, PE7-PE15, PD8-PD10 |
| 控制线 | PC0, PC2, PC3, PF11, PG8, PG15 |
| Byte Enable | PE0, PE1 |

关键注意：

- `NEU_Group05_Task.ioc` 里初始 SDRAM Column Bits 仍可能表现为 8 bit。
- `Core/Src/fmc.c` 中 `SDRAM_ForceApolloConfig()` 会强制改成 9 bit 并重新初始化。
- 重新生成代码后必须保留 `SDRAM_ForceApolloConfig()` 和 `SDRAM_Initialization_Sequence()`。

## 11. USART1 调试串口

| 信号 | MCU 引脚 | 配置 |
| --- | --- | --- |
| USART1_TX | PA9 | AF7 USART1 |
| USART1_RX | PA10 | AF7 USART1 |

串口参数：

| 项目 | 值 |
| --- | --- |
| BaudRate | 115200 |
| WordLength | 8 bit |
| StopBits | 1 |
| Parity | None |
| Flow Control | None |

`Core/Src/usart.c` 重定向了 `fputc()`，所以 `printf()` 会从 USART1 输出。

## 12. 时钟、定时器与中断

系统时钟：

| 项目 | 当前代码值 |
| --- | --- |
| SYSCLK | 180 MHz |
| PLL Source | HSI |
| PLLM | 8 |
| PLLN | 180 |
| PLLP | 2 |
| APB1 | HCLK / 4 = 45 MHz |
| APB2 | HCLK / 2 = 90 MHz |

注意：`NEU_Group05_Task.ioc` 中 HSE/LSE 引脚已配置，但 `SystemClock_Config()` 当前实际使用 HSI 作为 PLL 源。

定时器：

| 定时器 | 用途 | 配置 |
| --- | --- | --- |
| TIM7 | HAL timebase | FreeRTOS 下 SysTick/PendSV 优先级为 15 |
| TIM8 | 普通基础定时器 | Prescaler `18000 - 1`，Period `100 - 1`，TRGO Update |

中断：

| 中断 | 用途 | 优先级 |
| --- | --- | --- |
| EXTI3_IRQn | MAX30102_INT / PB3 | 5 |
| TIM7_IRQn | HAL Tick | 15 |
| PendSV/SysTick | FreeRTOS | 15 |

## 13. FreeRTOS 任务

| 任务 | 入口函数 | 优先级 | 栈大小 | 作用 |
| --- | --- | --- | --- | --- |
| Group05_DefTask | `StartDefaultTask` | Normal | 128 words | 每 16 ms 调用 `TouchGFX_SignalVSync()` |
| Group05_TGFX | `TouchGFX_Task` | AboveNormal | 1024 words | 运行 TouchGFX 主循环 |
| Group05_TInit | `taskTouchInit` | BelowNormal | 512 words | 延时初始化 GT9XXX 触摸，完成后退出 |
| Group05_Sensor | `StartTaskSensorService` | Normal | 1024 words | 根据页面状态采集 ECG、压力、PPG |

`freertos.c` 中还有一些 `Group05_` 前缀的演示任务函数，但当前未在 `MX_FREERTOS_Init()` 创建：

- `taskLED0`
- `taskKEYLED1`
- `taskKEY0`
- `taskKEY2`
- `taskMPS20`
- `taskMAX30102`

这些函数可作为调试参考，但不要误认为当前主流程正在运行。

## 14. TouchGFX MVP 数据流

当前主数据流：

```text
传感器驱动 / ADC / I2C
        |
        v
Core/Src/app_sensor_service.c
        |
        v
TouchGFX/gui/src/model/Model.cpp
        |
        v
Presenter
        |
        v
View / TouchGFX 控件
```

主要页面：

| 页面 | 功能 |
| --- | --- |
| HomeScreen | 入口页，跳转 ECG / Pulse / Pressure |
| ECGScreen | ECG 曲线、Raw ADC、导联脱落、LED 控制 |
| PulseScreen | 心率、血氧、IR/RED、采样进度 |
| PressureScreen | 压力数值和 ADC 曲线 |

`Model.cpp` 当前除了读 `app_sensor_service` 数据，还直接操作 LED、KEY0 和蜂鸣器。嵌入式项目中可以接受，但如果后续继续整理架构，可以把这些硬件操作再封装成独立 service。

## 15. CubeMX 重新生成代码注意事项

重新打开 `NEU_Group05_Task.ioc` 并生成代码前，建议重点检查：

1. `Core/Src/ltdc.c`
   - 保留 `LTDC_ForceApolloConfig()`。
   - 保留 `LTDC_ForceApolloPins()`。
2. `Core/Src/fmc.c`
   - 保留 `SDRAM_ForceApolloConfig()`。
   - 保留 `SDRAM_Initialization_Sequence()`。
   - 确认 SDRAM Column Bits 最终为 9 bit。
3. `Core/Inc/main.h`
   - KEY2、LED0、LED1 是 USER CODE 手写定义。
4. `Core/Src/gpio.c`
   - KEY2 当前 No Pull，如果硬件没有外部上拉，建议改为 Pull-up。
5. PB0/PB1
   - 当前代码把它们当 LED。
   - CubeMX 里仍可能显示 LTDC_R3/LTDC_R6，后续最好按实际板卡原理图彻底修正。
6. `TouchGFX/target/STM32TouchController.cpp`
   - 当前触摸初始化是延迟到 `Group05_TInit` 任务完成。
7. `app_ui.c`
   - 旧 framebuffer UI 已删除，当前 UI 以 TouchGFX 为准。

## 16. 新增或修改传感器建议流程

1. 先在 `NEU_Group05_Task.ioc` 中分配引脚和外设。
2. 在 `Core/Inc/main.h` 确认生成的 GPIO label。
3. 新建或修改驱动文件，避免把业务计算直接写进 TouchGFX View。
4. 在 `app_sensor_service.h` 的 `SensorData_t` 中增加状态字段。
5. 在 `app_sensor_service.c` 中加入采样周期、运行开关和更新函数。
6. 在 `Model.cpp` 中读取并通知 Presenter。
7. 在对应 Presenter 中做节流、状态判断、单位换算。
8. View 只负责控件显示和用户操作转发。
9. 用 USART1 先打印原始值，再接入 TouchGFX UI。

## 17. 快速接线清单

| 模块 | 信号 | MCU |
| --- | --- | --- |
| MAX30102 | SCL | PH4 / I2C2_SCL |
| MAX30102 | SDA | PH5 / I2C2_SDA |
| MAX30102 | INT | PB3 / EXTI3 |
| AD8232 | OUTPUT | PC1 / ADC1_IN11 |
| AD8232 | LO+ | PC4 |
| AD8232 | LO- | PC5 |
| MSP20 | Analog OUT | PA5 / ADC1_IN5 |
| GT9XXX | SCL | PH6 |
| GT9XXX | SDA | PI3 |
| GT9XXX | RST | PI8 |
| GT9XXX | INT | PH7 |
| LCD | Backlight | PB5 |
| KEY0 | Input | PH3 |
| KEY1 | Input | PH2 |
| KEY2 | Input | PC13 |
| WKUP | Input | PA0 |
| LED0 | Output | PB1 |
| LED1 | Output | PB0 |
| USART1 | TX | PA9 |
| USART1 | RX | PA10 |
