#include "matrix.h"
#include "stm32f1xx_hal.h"
#include "framebuffer.h"

unsigned int matrix_row;

#define GPIO_LATCH (1 << 9)
#define GPIO_OE (1 << 10)

DMA_HandleTypeDef hdma;
extern TIM_HandleTypeDef htim2;

uint32_t dma_ptr;
unsigned int prescaler;
unsigned int matrix_row;

void matrix_next() {
	
	int show = matrix_row;

  GPIOB->ODR = GPIO_OE | (matrix_row << 11) | GPIO_LATCH;

	dma_ptr += FRAMEBUFFER_SHIFTLEN * sizeof(uint16_t);
	
	if (prescaler == 0) {
		prescaler = (1 << FRAMEBUFFER_MAXBITDEPTH) - 1;
		++matrix_row;
		if (matrix_row == MATRIX_PANEL_SCANROWS) {
			matrix_row = 0;
			dma_ptr = (uint32_t)framebuffer_get();
		}
	}
	prescaler >>= 1;
	
	//HAL_DMA_Start(&hdma, dma_ptr, (uint32_t)&GPIOB->ODR, 1); //FRAMEBUFFER_SHIFTLEN);
	//HAL_DMA_PollForTransfer(&hdma, HAL_DMA_FULL_TRANSFER, 10000);
	
	for (int i = 0; i < FRAMEBUFFER_SHIFTLEN; ++i) {
		GPIOB->ODR = (show << 11) | ((uint16_t*)dma_ptr)[i];
		GPIOB->BSRR = (1 << 15);
	}
	GPIOB->ODR = GPIO_OE;
	
  htim2.Instance->CNT = 0;
	htim2.Instance->PSC = prescaler;
  htim2.Instance->CR1 |= TIM_CR1_CEN;
}

void matrix_start() {
	//Set up variables as if last data was just clocked in, matrix_next will actually fix things up for us.
	matrix_row = MATRIX_PANEL_SCANROWS - 1;
	prescaler = 0;
	matrix_next();
}

void matrix_setbrightness(uint8_t b)
{
	// todo
}

void DMA_Done(DMA_HandleTypeDef *hdma) {
	//If the timer has already stopped running, queue up the next row.
//	if (!(htim2.Instance->CR1 & TIM_CR1_CEN))
		matrix_next();
}

void matrix_init_timer() {
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 1;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 0x100;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
	htim2.Init.RepetitionCounter = 0;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	HAL_TIM_Base_Init(&htim2);
	
	/*
	TIM_OC_InitTypeDef oc_config = { 0 };
	oc_config.OCMode = TIM_OCMODE_PWM1;
	oc_config.Pulse = MATRIX_MINIMUM_DISPLAY_TIME;
	oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
	oc_config.OCFastMode = TIM_OCFAST_DISABLE;
	*/
	
	//HAL_TIM_OC_ConfigChannel(&htim2, &oc_config, TIM_CHANNEL_3);

	HAL_TIM_Base_Start_IT(&htim2); // TIM_CHANNEL_3);
}

void matrix_init_dma() {
  __HAL_RCC_DMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	hdma.Instance = DMA1_Channel1;
	hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma.Init.MemInc = DMA_MINC_ENABLE;
	hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma.Init.Mode = DMA_NORMAL;
	hdma.Init.Priority = DMA_PRIORITY_VERY_HIGH;
	
	HAL_DMA_Init(&hdma);
	HAL_DMA_RegisterCallback(&hdma, HAL_DMA_XFER_CPLT_CB_ID, DMA_Done);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	//Disable timer
	htim2.Instance->CR1 &= ~TIM_CR1_CEN;
//	if (!(hdma.Instance->CCR & DMA_CCR_EN))
		matrix_next();
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
	//Disable timer
	htim2.Instance->CR1 &= ~TIM_CR1_CEN;
//	if (!(hdma.Instance->CCR & DMA_CCR_EN))
		matrix_next();
}

void matrix_init() {
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = 0xff3b;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = (1 << 1);
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

	matrix_init_timer();
  //matrix_init_dma();
  matrix_start();

}
