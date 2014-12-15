#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "misc.h"
#include <stdio.h>
#include <stdint.h>

void USART_puts				(USART_TypeDef* USARTx, volatile char *c);
void USART_init				(void);
void Timer_init  			(void);
void myDelay				(uint32_t uS);

static char str_buffer[50];

int main(void)
{
  Timer_init();
  USART_init();

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 |GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  uint32_t timerValue;
  uint32_t sensorValue = 0;
  uint32_t result_1;
  float result_2;

  uint16_t i;
	while (1) {
		sensorValue = 0;
		i = 10000;

		GPIO_ResetBits(GPIOD, GPIO_Pin_12);
		//timerValue = TIM2->CNT;

		while (i--) {
			result_1 = ((sensorValue * 17) / 100);
			sensorValue++;
			sprintf(str_buffer, "int result = %d.%d \r\n", result_1/10,result_1%100);
		}

		GPIO_SetBits(GPIOD, GPIO_Pin_12);
		//timerValue = TIM2->CNT - timerValue;

		//USART_puts(USART1, str_buffer);
    	//sprintf(str_buffer, "Time int : %u uSec\r\n",(unsigned int)timerValue );
    	//USART_puts(USART1, str_buffer);

    	myDelay( 1000 );

    	sensorValue = 0;
		i = 10000;

		//timerValue = TIM2->CNT;
		GPIO_ResetBits(GPIOD, GPIO_Pin_13);
		while (i--) {
			result_2 = ((sensorValue * 17.0) / 1000);
			sensorValue++;
			sprintf(str_buffer, "float result = %.2f \r\n", result_2);
		}
		GPIO_SetBits(GPIOD, GPIO_Pin_13);
		//timerValue = TIM2->CNT - timerValue;

		//USART_puts(USART1, str_buffer);
    	//sprintf(str_buffer, "Time float : %u uSec\r\n\r\n",(unsigned int)timerValue );
    	//USART_puts(USART1, str_buffer);
    	myDelay( 5000000 );
	}
}
void myDelay(uint32_t uS)
{
    uint32_t timerValue;
    uint32_t lastTime = TIM2->CNT;
    while(1)
    {
    	timerValue = TIM2->CNT;
    	if( timerValue - lastTime > uS )
    	{
    		break;
    	}
    }
    return;
}
// Timer count 1uS
void Timer_init(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseInitTypeDef timerInitStructure;
  timerInitStructure.TIM_Prescaler = 84 - 1;               // Timer Clock 84MHz
  timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  timerInitStructure.TIM_Period = 0xFFFFFFFF - 1;
  timerInitStructure.TIM_RepetitionCounter = 0;
  timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &timerInitStructure);
  TIM_Cmd(TIM2, ENABLE);
}

void USART_puts(USART_TypeDef* USARTx, volatile char *c)
{
  while(*c){
    // wait until data register is empty
    while( !(USARTx->SR & 0x00000040) );
    USART_SendData(USARTx, *c);
    *c++;
  }
}

void USART_init(void)
{
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);

  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_BaudRate              = 115200;
  USART_InitStructure.USART_WordLength            = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits              = USART_StopBits_1;
  USART_InitStructure.USART_Parity                = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl   = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode                  = USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);

    /* Enable USART1 */
  USART_Cmd(USART1, ENABLE);
}
