/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_I2C1_Init();
 /* USER CODE BEGIN 2 */
  // 1. 配置 CAN 数据过滤器（全通模式，接收所有 CAN 帧）
  CAN_FilterTypeDef sFilterConfig;
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;

  HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);

  // 2. 启动 CAN 外设
  HAL_CAN_Start(&hcan);
/* USER CODE END 2 */


  

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 所有逻辑由 CAN RX 中断驱动，主循环空闲 */
    /* USER CODE END WHILE */
  }



    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  将 CAN 帧中的模拟量数据格式化为 FireWater CSV 行
  * @param  buf:     输出缓冲区
  * @param  maxlen:  缓冲区最大长度
  * @param  canId:   收到的 CAN 标准 ID
  * @param  data:    CAN 帧数据（每 2 字节大端 = 1 个通道原始 ADC 值）
  * @param  dlc:     CAN 帧数据长度
  * @retval 写入缓冲区的字节数（含结尾 '\n'），失败返回 -1
  */
int format_firewater(char *buf, int maxlen, uint32_t canId, uint8_t *data, uint8_t dlc)
{
  if (maxlen < 16) return -1;  /* 缓冲区太小 */

  int pos = 0;

  /* 第 1 列：系统运行毫秒时间戳，第 2 列：CAN ID（数据来源标识） */
  pos = snprintf(buf, maxlen, "%lu,0x%03lX", (unsigned long)HAL_GetTick(), (unsigned long)canId);

  /* 后续列：每 2 字节解析为一个通道的原始 ADC 值（大端序，12 位） */
  /* 转换为电压值（V），使用整数运算避免 microlib 浮点格式问题 */
  for (int i = 0; i + 1 < (int)dlc; i += 2)
  {
    uint16_t raw = ((uint16_t)data[i] << 8) | data[i + 1];
    /* raw * 3.3 / 4096 → 转换为毫伏: raw * 3300 / 4096 */
    uint16_t mv = (uint16_t)(((uint32_t)raw * 3300UL + 2048UL) / 4096UL);
    int int_part = mv / 1000;
    int frac_part = mv % 1000;

    if (pos < maxlen - 10)
    {
      pos += snprintf(buf + pos, maxlen - pos, ",%d.%03d", int_part, frac_part);
    }
    else
    {
      break;  /* 缓冲区不足，停止追加 */
    }
  }

  /* 行尾换行符（FireWater 协议要求） */
  if (pos < maxlen - 1)
  {
    buf[pos++] = '\n';
  }
  buf[pos] = '\0';

  return pos;
}

/**
  * @brief  将缓冲数据按 CAN 多帧分段协议发送
  * @param  hcan:   CAN 句柄指针
  * @param  stdId:  目标 CAN 标准 ID
  * @param  data:   待发送数据
  * @param  len:    数据字节数
  * @note   分段协议：Byte0 为控制字
  *         - 起始帧: 0x80 | 总字节数, Byte1-7 = 前 7 字节数据
  *         - 中间帧: 序号(0-126), Byte1-7 = 7 字节数据
  *         - 结束帧: 0xFF, Byte1-7 = 剩余数据 + 补 0
  *         总长 ≤ 7 时直接发结束帧（单帧）
  */
void can_send_segmented(CAN_HandleTypeDef *hcan, uint32_t stdId, uint8_t *data, int len)
{
  CAN_TxHeaderTypeDef txHeader = {0};
  uint8_t txData[8];
  uint32_t mailbox;
  int offset = 0;
  int seq = 0;

  txHeader.StdId = stdId;
  txHeader.ExtId = 0x00;
  txHeader.RTR  = CAN_RTR_DATA;
  txHeader.IDE  = CAN_ID_STD;
  txHeader.DLC  = 8;
  txHeader.TransmitGlobalTime = DISABLE;

  if (len <= 0) return;

  /* 单帧：总长 ≤ 7，直接发结束帧 */
  if (len <= 7)
  {
    txData[0] = 0xFF;
    (void)memcpy(&txData[1], data, len);
    (void)memset(&txData[1 + len], 0, 7 - len);
    HAL_CAN_AddTxMessage(hcan, &txHeader, txData, &mailbox);
    return;
  }

  /* 起始帧：Byte0 = 0x80 | 总字节数 */
  txData[0] = 0x80 | ((uint8_t)len & 0x7F);
  (void)memcpy(&txData[1], data, 7);
  HAL_CAN_AddTxMessage(hcan, &txHeader, txData, &mailbox);
  offset = 7;

  /* 中间帧：序号递增 */
  while (offset + 7 < len)
  {
    txData[0] = (uint8_t)(seq++);
    (void)memcpy(&txData[1], data + offset, 7);
    HAL_CAN_AddTxMessage(hcan, &txHeader, txData, &mailbox);
    offset += 7;
  }

  /* 结束帧 */
  txData[0] = 0xFF;
  int remain = len - offset;
  (void)memcpy(&txData[1], data + offset, remain);
  (void)memset(&txData[1 + remain], 0, 7 - remain);
  HAL_CAN_AddTxMessage(hcan, &txHeader, txData, &mailbox);
}

/**
  * @brief  CAN RX FIFO0 消息挂起回调
  *         收到模拟 I/O 模块的 CAN 帧后，格式化为 FireWater 并分段发送
  * @param  hcan: CAN 句柄指针
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t rxData[8];

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
  {
    return;
  }

  /* 仅处理模拟 I/O 模块的 CAN ID 范围 (0x100 - 0x10F) */
  if (rxHeader.StdId < CAN_ID_ANALOG_BASE || rxHeader.StdId > CAN_ID_ANALOG_END)
  {
    return;
  }

  /* 格式化为 FireWater 行 */
  char fw_buf[FW_BUF_SIZE];
  int len = format_firewater(fw_buf, FW_BUF_SIZE, rxHeader.StdId, rxData, rxHeader.DLC);
  if (len > 0)
  {
    can_send_segmented(hcan, CAN_ID_HOST_OUTPUT, (uint8_t *)fw_buf, len);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
