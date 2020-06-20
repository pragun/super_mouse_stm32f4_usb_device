/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <circular_buffers.hpp>
#include "main.h"
#include "usb_device.h"
#include "usbd_hid.h"
#include <cstdio>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim9;
TIM_HandleTypeDef htim10;
TIM_HandleTypeDef htim11;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */

int tim11_count = 0;
int tim10_count = 0;
int tim9_count = 0;
int uart_rx_count = 0;
int usb_event_num = 0;
int mouse_event_num = 0;
int keyboard_event_num = 0;
int gpio_pa8_interrupt_count = 0;

#define UART_RX_BUF_SIZE 128
#define UART_TX_BUF_SIZE 512
#define SPI_RX_BUF_SIZE 17

#define USING_DMA_UART 1
#define USING_CIRCULAR_DMA 1

int spi_rx_count = 0;
int spi_error_count = 0;
int spi_half_rx_count = 0;
const uint16_t keep_alive_period = 50'000;
uint32_t uart_rx_dma_remaining_bytes = 0;

int16_t accumulated_mouse_del_x = 0;
int16_t accumulated_mouse_del_y = 0;

int16_t accumulated_scroll_x = 0;
int16_t accumulated_scroll_y = 0;

uint8_t current_primary_button_state = 0;
uint8_t previous_primary_button_state = 0;

uint16_t current_keypad_button_state = 0;
uint16_t previous_keypad_button_state = 0;

int16_t del_x = 0;
int16_t del_y = 0;
int8_t del_z = 0;

typedef struct
{
    uint8_t report_id = 1;
	uint8_t buttons;
    int16_t mouse_x;
    int16_t mouse_y;
    int8_t scroll_y;
    int8_t scroll_x;
}
Mouse_HID_Report_TypeDef;

typedef struct
{
    uint8_t report_id = 3;
	uint8_t buttons;
    uint16_t mouse_x;
    uint16_t mouse_y;
}
Absolute_Mouse_HID_Report_TypeDef;

Mouse_HID_Report_TypeDef mouse_hid_report;
Absolute_Mouse_HID_Report_TypeDef absolute_mouse_hid_report;

#pragma pack(1)
typedef struct
{
	int16_t dx;
	int16_t dy;
	int8_t dz;
	uint32_t buttons;
}
SPI_MMO_Mouse_State_TypeDef;

SPI_MMO_Mouse_State_TypeDef* spi_mouse_state_rx;

uint8_t rx_buf_read_pos = 0;
uint8_t rx_buf[UART_RX_BUF_SIZE];

uint8_t spi_rx_buf[] = "Test Test Test Test Test ";
UART_Tx_CircularBuffer uart2_tx_buf;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM9_Init(void);
static void MX_TIM10_Init(void);
static void MX_TIM11_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//extern "C" uint8_t PrintHexBuf(uint8_t *buff, uint8_t len);
uint8_t PrintHexBuf(uint8_t *buff, uint8_t len){
	for(uint8_t i = 0; i<len; i++){
		printf("%02X ",buff[i]);
		if ((i+1)%8 == 0){
			printf("\r\n");
		}
	}
	printf("\r\n");
	return 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	gpio_pa8_interrupt_count ++ ;
	//HAL_SPI_Receive_DMA(&hspi1, spi_rx_buf, 9);
	printf("GPIO Interrupt received.\r\n");
}

void spi_rx_complete(SPI_HandleTypeDef *hspi){
	spi_rx_count ++ ;

	printf("SPI RX : %d\r\n",spi_rx_count);
	PrintHexBuf(spi_rx_buf,9);

	spi_mouse_state_rx = (SPI_MMO_Mouse_State_TypeDef*) spi_rx_buf;

	accumulated_mouse_del_x += spi_mouse_state_rx->dx;
	accumulated_mouse_del_y += spi_mouse_state_rx->dy;
	accumulated_scroll_y += spi_mouse_state_rx->dz;

	current_primary_button_state = spi_mouse_state_rx->buttons & 0x0F;
	current_keypad_button_state = (spi_mouse_state_rx->buttons >> 8) & 0x0FFF;

	//spi_rx_buf[SPI_RX_BUF_SIZE] = 0;
	//printf("Rcv:{%s}\r\n",spi_rx_buf);

	printf("X:%d Y:%d Z:%d B:0x%x \r\n",accumulated_mouse_del_x,accumulated_mouse_del_y,accumulated_scroll_y,current_primary_button_state);
}

void spi_half_rx_complete(SPI_HandleTypeDef *hspi){
	spi_half_rx_count ++ ;
	printf("SPI Half Received: %d\r\n",spi_rx_count);
}

void spi_rx_error(SPI_HandleTypeDef *hspi){
	spi_error_count ++ ;
	printf("SPI Error: %d\r\n",spi_error_count);
}

void timer11_period_elapsed(TIM_HandleTypeDef *htim){
	tim11_count ++;
	printf("Client alive msg.. %d\r\n",tim11_count);

	//HAL_SPI_Receive_DMA(&hspi1, spi_rx_buf, SPI_RX_BUF_SIZE);
	//HAL_SPI_Receive_IT(&hspi1, spi_rx_buf, SPI_RX_BUF_SIZE);
	//CDC_Transmit_FS((uint8_t *) "Hello world \r\n",14);
}

void process_transfer_uart_rx_buf(uint8_t remaining_bytes){
	if ((UART_RX_BUF_SIZE - remaining_bytes)> 0){
		uint8_t bytes_to_transfer = UART_RX_BUF_SIZE - remaining_bytes - rx_buf_read_pos;
		uart2_tx_buf.write_to_queue((char *) &rx_buf[rx_buf_read_pos], bytes_to_transfer);
		rx_buf_read_pos += bytes_to_transfer;
	}
	if(remaining_bytes == 0){
		rx_buf_read_pos = 0;
	}
}

void timer10_period_elapsed(TIM_HandleTypeDef *htim){
	tim10_count ++;

	uart_rx_dma_remaining_bytes = __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
	process_transfer_uart_rx_buf(uart_rx_dma_remaining_bytes);

	if ((uart2_tx_buf.length_of_ongoing_transmission() == 0) && (uart2_tx_buf.length_of_queue() > 0)){
			auto [ tx_buf, tx_count ] = uart2_tx_buf.longest_possible_send();
			HAL_UART_Transmit_DMA(&huart2,(uint8_t*) tx_buf, tx_count);
	}
}


void timer9_period_elapsed(TIM_HandleTypeDef *htim){
	tim9_count ++;

	if (current_keypad_button_state != previous_keypad_button_state){
		absolute_mouse_hid_report.buttons = 0x01;
		absolute_mouse_hid_report.mouse_x = 5000;
		absolute_mouse_hid_report.mouse_y = 5000;
		USBD_HID_SendReport (&hUsbDeviceFS, (uint8_t*) &absolute_mouse_hid_report, 6);
		previous_keypad_button_state = current_keypad_button_state;
	}

	if ((accumulated_mouse_del_x != 0)||(accumulated_mouse_del_y != 0)||(accumulated_scroll_y !=0)||(previous_primary_button_state != current_primary_button_state)){
		mouse_hid_report.mouse_x = accumulated_mouse_del_x;
		mouse_hid_report.mouse_y = accumulated_mouse_del_y;
		mouse_hid_report.scroll_x = accumulated_scroll_y;
		mouse_hid_report.scroll_y = 0;
		mouse_hid_report.buttons = current_primary_button_state;
		previous_primary_button_state = current_primary_button_state;
		USBD_HID_SendReport (&hUsbDeviceFS, (uint8_t*) &mouse_hid_report, 8);

		accumulated_mouse_del_x = 0;
		accumulated_mouse_del_y = 0;
		accumulated_scroll_y = 0;
		return;
	}
}


void uart_rx_complete(UART_HandleTypeDef *huart){
	process_transfer_uart_rx_buf(0);
	uart_rx_count ++;
}

void uart_tx_complete(UART_HandleTypeDef *huart){
	uart2_tx_buf.last_send_complete();
}


extern "C" int _write(int file, char *ptr, int len);
int _write(int file, char *ptr, int len)
{
	uart2_tx_buf.write_to_queue(ptr, len);
	return len;
}


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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM9_Init();
  MX_TIM10_Init();
  MX_TIM11_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_RegisterCallback(&htim11,HAL_TIM_PERIOD_ELAPSED_CB_ID, timer11_period_elapsed);
  HAL_TIM_RegisterCallback(&htim10,HAL_TIM_PERIOD_ELAPSED_CB_ID, timer10_period_elapsed);
  HAL_TIM_RegisterCallback(&htim9,HAL_TIM_PERIOD_ELAPSED_CB_ID, timer9_period_elapsed);

//  HAL_UART_RegisterCallback(&huart2, HAL_UART_TX_COMPLETE_CB_ID, uart_transfer_completed);
  HAL_UART_RegisterCallback(&huart2, HAL_UART_RX_COMPLETE_CB_ID, uart_rx_complete);
  HAL_UART_RegisterCallback(&huart2, HAL_UART_TX_COMPLETE_CB_ID, uart_tx_complete);
  HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_RX_COMPLETE_CB_ID, spi_rx_complete);
  //HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_RX_HALF_COMPLETE_CB_ID, spi_half_rx_complete);
  HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_ERROR_CB_ID, spi_rx_error);

  HAL_TIM_Base_Start_IT(&htim11);
  HAL_TIM_Base_Start_IT(&htim10);
  HAL_TIM_Base_Start_IT(&htim9);

  /* USER CODE END 2 */
  HAL_UART_Receive_DMA(&huart2, rx_buf, UART_RX_BUF_SIZE);
  HAL_SPI_Receive_DMA(&hspi1, spi_rx_buf, 9);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t pI = 0;
  while (1)
  {
    /* USER CODE END WHILE */
	  HAL_Delay(3000);
	   pI =  USBD_HID_GetPollingInterval(&hUsbDeviceFS);

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 8400;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 0;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 8400;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 100;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */
}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{
  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 8400;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 1;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */
}

/**
  * @brief TIM11 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM11_Init(void)
{

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 8400;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = keep_alive_period;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 1000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
