#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_exti.h"
#include "misc.h"
#include <stdio.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

void GPIO_init				(void);
void USART_puts				(USART_TypeDef* USARTx, volatile char *c);
void USART_init				(void);

char str_value[20];

xQueueHandle myQueue;
xQueueHandle textQueue;

void vTaskA (void* pvParameters){
    for (;;) {
    	vTaskDelay(1000);
    	GPIOD->ODR ^= GPIO_Pin_12;
    }
}
void vTaskB (void* pvParameters){
    for (;;) {
    	vTaskDelay(500);
    	GPIOD->ODR ^= GPIO_Pin_13;
    }
}

void vTaskC (void* pvParameters){
	int delayLED = 1000;
	for (;;) {
		xQueueSend(myQueue, &delayLED, 0);
		vTaskDelay(1000);
		GPIO_ToggleBits(GPIOD,GPIO_Pin_14);

	}
}

void vTaskD (void* pvParameters){
	int _delayLED;
	for (;;) {

		xQueueReceive(myQueue, &_delayLED, portMAX_DELAY);

		vTaskDelay(_delayLED);
		GPIOD->ODR ^= GPIO_Pin_15;
	}
}

void vTaskE (void* pvParameters){
	for (;;) {
		vTaskDelay(1000);
		xQueueSend(textQueue, "queue it's work!\r\n", 0);
	}
}

void vTaskF (void* pvParameters){
	char _msg[30];
	for (;;) {
		xQueueReceive(textQueue, _msg, portMAX_DELAY);
		USART_puts(USART1, _msg);
	}
}

int main(void)
{

	GPIO_init();
	USART_init();

	sprintf(str_value, "\r\nHello World!\r\n" );
	USART_puts(USART1, str_value);

	myQueue = xQueueCreate( 10, sizeof(int) );
	textQueue = xQueueCreate( 10, sizeof(char[30]));

	xTaskCreate(vTaskA, "Task 1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskB, "Task 2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskC, "Task 3", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskD, "Task 4", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskE, "Task 5", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskF, "Task 6", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	sprintf(str_value, "Task Created\r\n" );
	USART_puts(USART1, str_value);

	vTaskStartScheduler();

	for(;;);

	return 0;
}

void USART1_IRQHandler(void){

  if( USART_GetITStatus(USART1, USART_IT_RXNE) )
  {
    char c = USART1->DR;
    USART_puts(USART1, &c);
  }
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
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_BaudRate              = 115200;
  USART_InitStructure.USART_WordLength            = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits              = USART_StopBits_1;
  USART_InitStructure.USART_Parity                = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl   = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode                  = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(USART1, &USART_InitStructure);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

    /* Enable USART1 */
  USART_Cmd(USART1, ENABLE);
}


void GPIO_init(void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_WriteBit(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15, Bit_RESET);
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	taskDISABLE_INTERRUPTS();
	for(;;);
}
void vApplicationMallocFailedHook(void) {
	taskDISABLE_INTERRUPTS();
	for(;;);
}
void vApplicationTickHook(void) {
}
void vApplicationIdleHook(void) {
}
