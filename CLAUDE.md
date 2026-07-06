# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

激光光功率测量系统，基于 STM32F103C8T6 微控制器。通过 I2C 读取光功率传感器数据，经 CAN 总线发送至采集板。项目由 STM32CubeMX 6.15.0 生成，使用 STM32Cube FW_F1 V1.8.7 固件包。

## 构建与烧录

- **IDE**: Keil MDK-ARM V5.32，项目文件为 [MDK-ARM/laser.uvprojx](MDK-ARM/laser.uvprojx)
- **编译**: 在 Keil IDE 中打开 `.uvprojx` 文件，点击 Build (F7)
- **烧录/调试**: 通过 SWD 接口 (PA13/SWDIO, PA14/SWCLK) 连接 ST-Link 或 J-Link，在 Keil 中点击 Download (F8) 或 Start Debug (Ctrl+F5)
- **CubeMX 重新生成**: 打开 [laser.ioc](laser.ioc)，修改配置后点击 GENERATE CODE。项目设置了 `KeepUserCode=true`，USER CODE BEGIN/END 块内的自定义代码会被保留

## 硬件引脚映射

| 外设  | 引脚 | 功能      |
|-------|------|-----------|
| CAN   | PA11 | CAN_RX    |
| CAN   | PA12 | CAN_TX    |
| I2C1  | PB6  | I2C1_SCL  |
| I2C1  | PB7  | I2C1_SDA  |
| SWD   | PA13 | SWDIO     |
| SWD   | PA14 | SWCLK     |
| HSE   | PD0  | OSC_IN    |
| HSE   | PD1  | OSC_OUT   |

## 架构

### 代码组织

本项目遵循 STM32CubeMX 生成的标准 HAL 库结构：

- `Core/Src/` — 用户应用代码（main.c、外设初始化、中断服务）
- `Core/Inc/` — 用户头文件
- `Drivers/STM32F1xx_HAL_Driver/` — STM32 HAL 库（只读，勿手动修改）
- `Drivers/CMSIS/` — Cortex-M3 CMSIS 核心文件（只读）
- `MDK-ARM/` — Keil 工程文件、启动汇编、链接脚本

### 初始化流程

1. `HAL_Init()` — 初始化 HAL 库和 SysTick
2. `SystemClock_Config()` — 配置 HSE (8MHz) → PLL ×9 → 系统时钟 72MHz，APB1 36MHz，APB2 72MHz
3. `MX_GPIO_Init()` — GPIO 时钟使能
4. `MX_CAN_Init()` — CAN 外设初始化（Prescaler=16, BS1=1TQ, BS2=1TQ, 约 750kbps）
5. `MX_I2C1_Init()` — I2C1 外设初始化（标准模式 100kHz，7位地址）

### USER CODE 区域

STM32CubeMX 使用 `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` 注释来保护用户代码。重新生成项目时，只有这些标记之间的代码会被保留。**所有自定义代码必须写在这些标记块内。**

### 当前开发状态

`main.c` 中的主循环目前是 CAN 通信测试：每 100ms 发送一个 8 字节固定测试帧（StdId=0x123, DLC=8, 数据: AA 55 12 34 56 78 9A BC）。CAN 过滤器配置为接收全部报文（Mask=0x00000000）。I2C1 仅初始化，尚未挂接具体的传感器读数逻辑。中断服务函数 [stm32f1xx_it.c](Core/Src/stm32f1xx_it.c) 中所有外设中断均为默认（空实现），当前仅 SysTick 中断有实际逻辑（调用 `HAL_IncTick()`）。

### 编译宏

`USE_HAL_DRIVER`, `STM32F103xb`（在 [.mxproject](.mxproject) 的 CDefines 中定义，Keil 工程自动继承）

### HAL 模块使能

在 [Core/Inc/stm32f1xx_hal_conf.h](Core/Inc/stm32f1xx_hal_conf.h) 中启用：CAN, GPIO, I2C, CORTEX, DMA, FLASH, EXTI, PWR, RCC
