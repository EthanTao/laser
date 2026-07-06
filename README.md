# 激光光功率测量系统

基于 STM32F103C8T6 的嵌入式 CAN 数据采集与转发系统。通过 CAN 总线接收模拟 I/O 模块的电信号数据，解析后以 FireWater 协议格式分段发送至上位机 VOFA+ 进行可视化显示。

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz) |
| 开发板 | 定制板 / Blue Pill 兼容 |
| 调试接口 | SWD (PA13/SWDIO, PA14/SWCLK) |
| 外部晶振 | HSE 8MHz |
| 系统时钟 | 72MHz (HSE → PLL ×9) |

## 引脚映射

| 外设 | 引脚 | 功能 | 说明 |
|------|------|------|------|
| CAN | PA11 | CAN_RX | 连接 CAN 收发器 (TJA1050 等) |
| CAN | PA12 | CAN_TX | 连接 CAN 收发器 |
| I2C1 | PB6 | I2C1_SCL | 预留，连接光功率传感器 |
| I2C1 | PB7 | I2C1_SDA | 预留，连接光功率传感器 |
| SWD | PA13 | SWDIO | 调试 / 烧录 |
| SWD | PA14 | SWCLK | 调试 / 烧录 |
| HSE | PD0 | OSC_IN | 外部 8MHz 晶振 |
| HSE | PD1 | OSC_OUT | 外部 8MHz 晶振 |

## 系统架构

```
┌──────────────┐    CAN 总线     ┌──────────────┐    CAN 总线     ┌───────────────┐    UART    ┌──────────┐
│ 模拟 I/O 模块 │ ──────────────→ │  STM32F103   │ ──────────────→ │ CAN-UART 转换器 │ ────────→ │ PC/VOFA+ │
│ (CAN ID:      │                │              │  分段 FireWater │               │          │          │
│  0x100-0x10F) │                │  解析+格式化   │   (CAN ID:0x124)│               │          │          │
└──────────────┘                └──────────────┘                 └───────────────┘          └──────────┘
```

## 通信协议

### CAN 参数

| 参数 | 值 |
|------|-----|
| 波特率 | 750 kbps |
| 帧格式 | 标准帧 (11-bit ID) |
| 预分频 | 16 |
| 时间片 | BS1=1TQ, BS2=1TQ |

### CAN ID 分配

| ID 范围 | 用途 |
|---------|------|
| `0x100` - `0x10F` | 模拟 I/O 模块发送的原始数据 |
| `0x124` | STM32 输出 FireWater 数据到上位机 |

### CAN 多帧分段协议

FireWater 单行文本（约 15-40 字节）超过 CAN 8 字节上限时，采用以下分段协议：

| 帧类型 | Byte 0 编码 | Byte 1-7 |
|--------|-----------|---------|
| 起始帧 | `0x80 \| 总字节数` | 前 7 字节数据 |
| 中间帧 | `序号 (0-126)` | 随后 7 字节数据 |
| 结束帧 | `0xFF` | 剩余数据 + 零填充 |

单帧（≤ 7 字节）直接以结束帧发送。

**示例** — 发送 `12345,0x100,2.50,3.30\n`（24 字节）：

```
帧 1 (起始): [0x98] [1][2][3][4][5][,][0]
帧 2 (中间): [0x00] [x][1][0][0][,][2][.]
帧 3 (中间): [0x01] [5][0][,][3][.][3][0]
帧 4 (结束): [0xFF] [\n][0][0][0][0][0][0]
```

### FireWater 数据格式（CSV）

```
<时间戳ms>,<CAN来源ID>,<通道1电压V>,<通道2电压V>,...\n
```

示例输出：`12345,0x100,2.50,3.30,1.80\n`

每个通道值为 12-bit ADC 转换为电压（3.3V 参考，4096 分辨率）。

## 开发环境

| 工具 | 版本 |
|------|------|
| IDE | Keil MDK-ARM V5.32 |
| 代码生成 | STM32CubeMX 6.15.0 |
| HAL 固件包 | STM32Cube FW_F1 V1.8.7 |
| 编译器 | ARM Compiler 5 / 6 |
| 调试器 | ST-Link / J-Link (SWD) |

## 构建与烧录

```bash
# 方法 1：Keil IDE（推荐）
# 1. 双击打开 MDK-ARM/laser.uvprojx
# 2. 点击 Build (F7) 编译
# 3. 点击 Download (F8) 烧录

# 方法 2：命令行（需安装 Keil 和 armcc 到 PATH）
# 编译
cd MDK-ARM
"C:\Keil_v5\UV4\UV4.exe" -b laser.uvprojx -j0 -o build.log

# 烧录（需 ST-Link Utility）
ST-LINK_CLI.exe -c SWD -P laser.hex -V
```

### CubeMX 重新生成

修改 `laser.ioc` 后点击 **GENERATE CODE** 重新生成项目。项目已设置 `KeepUserCode=true`，所有 `USER CODE BEGIN/END` 块内的自定义代码会被保留。

> ⚠️ 注意：自定义代码必须写在 `/* USER CODE BEGIN xxx */` 和 `/* USER CODE END xxx */` 之间，否则会被 CubeMX 覆盖。

## 项目结构

```
laser/
├── laser.ioc              # STM32CubeMX 项目配置文件
├── .mxproject             # CubeMX 项目元数据
├── Core/
│   ├── Inc/               # 用户头文件
│   │   ├── main.h         # 主头文件（CAN ID 宏定义、函数声明）
│   │   ├── can.h          # CAN 外设句柄声明
│   │   ├── i2c.h          # I2C 外设句柄声明
│   │   ├── gpio.h         # GPIO 初始化声明
│   │   ├── stm32f1xx_it.h # 中断服务声明
│   │   └── stm32f1xx_hal_conf.h # HAL 模块配置
│   └── Src/               # 用户源文件
│       ├── main.c         # 主程序（FireWater 格式化、CAN 分段发送、RX 回调）
│       ├── can.c          # CAN 初始化 + NVIC 配置
│       ├── i2c.c          # I2C 初始化
│       ├── gpio.c         # GPIO 初始化
│       ├── stm32f1xx_it.c # 中断服务（USB_LP_CAN1_RX0_IRQHandler）
│       └── stm32f1xx_hal_msp.c # HAL MSP 初始化
├── Drivers/               # STM32 HAL 库 & CMSIS（只读，勿手动修改）
│   ├── STM32F1xx_HAL_Driver/
│   └── CMSIS/
└── MDK-ARM/               # Keil 工程文件
    ├── laser.uvprojx       # Keil 项目文件
    ├── laser.uvoptx       # Keil 项目选项
    └── startup_stm32f103xb.s # 启动汇编
```

## 代码关键函数

| 函数 | 文件 | 功能 |
|------|------|------|
| `HAL_CAN_RxFifo0MsgPendingCallback()` | main.c | CAN RX 中断回调，过滤 ID 后触发格式化 |
| `format_firewater()` | main.c | 解析 CAN 帧模拟量，生成 FireWater CSV 行 |
| `can_send_segmented()` | main.c | CAN 多帧分段发送（起始/中间/结束帧协议） |
| `USB_LP_CAN1_RX0_IRQHandler()` | stm32f1xx_it.c | CAN RX0 中断入口（STM32F103 与 USB LP 共享） |

## 上位机配置 (VOFA+)

1. 下载 VOFA+：https://www.vofa-plus.com/
2. 选择 COM 口（CAN-UART 转换器对应的端口），波特率与转换器一致
3. 协议选择 **FireWater**
4. 横轴选择时间戳（第 1 列），纵轴选择各通道电压值
5. 点击连接即可查看实时波形

> 如果 CAN-UART 转换器透传了控制字节（起始/中间/结束帧标记），导致 VOFA+ 无法解析，需要一个 PC 端脚本剥离控制字节并重组 FireWater 行。脚本模板见 `tools/can_reassembler.py`。

## 许可证

本项目基于 STM32CubeMX 生成，HAL 库和 CMSIS 部分版权归 STMicroelectronics 所有。自定义应用代码采用 MIT License。
