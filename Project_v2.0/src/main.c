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

void ADC_init			(void);
void Timer_init			(void);
void GPIO_init			(void);
void EXTI_init			(void);
void USART_init			(void);
void USART_puts			(USART_TypeDef* USARTx, volatile char *c);
void uDelay				(uint32_t uS);
void enableExtiROS		(void);

#define US_NUMBER 2
#define US_10Hz 10
#define US_BUFFER_SIZE (US_NUMBER*US_10Hz)

#define ROS_NUMBER 4
#define ROS_10Hz 10
#define ROS_BUFFER_SIZE (ROS_NUMBER*ROS_10Hz)

#define ADC_NUMBER 5
#define ADC_50Hz 50
#define ADC_BUFFER_SIZE (ADC_NUMBER*ADC_50Hz)

typedef enum { start, ping, wait_risingEcho, wait_fallingEcho, end } US_State;
typedef enum { false, true } bool;

struct us_profile {
	uint8_t id;
	uint8_t status;		// 0 normal, 1 none ack, 2 distance_out_of_range
	uint32_t distance;
};

struct ros_profile {
	uint8_t id;
	uint32_t speed;
};

struct adc_profile {
	uint8_t id;
	uint32_t volt;
};

xTaskHandle xUS_1;
xTaskHandle xUS_2;
xQueueHandle usQueue;
xQueueHandle rosQueue;
xQueueHandle adcQueue;

//global string buffer
char str_buffer[100];

//dma buffer for adc 5ch
extern uint16_t ADCValue[5];

//variable for process reflective optical sensor
uint8_t roundFlag[4];
uint32_t roundTime[4] = {0};
uint32_t roundLastTime[4] = {0};

//flag variable for ultrasonic sensor
bool usFlag1 = false;
bool usFlag2 = false;

struct us_profile us_data[US_BUFFER_SIZE] = {0};
struct ros_profile ros_data[ROS_BUFFER_SIZE] = {0};
struct adc_profile adc_data[ADC_BUFFER_SIZE] = {0};

//heart beat system
void vTaskBlink (void* pvParameters)
{
	for (;;)
	{
		vTaskDelay(500);
		GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
	}
}

//task ultrasonic signal process #1
void vTaskUS_1 (void* pvParameters)
{
	/* timer vatriable */
    uint32_t timerState;
    uint32_t currentTime, lastTime = TIM2->CNT;
    /* for calculate distance */
    uint32_t raw, distance;
    /* buffer for send to queue */
    struct us_profile this_us = {1,0,0};
    US_State state = start;

    for (;;)
    {
		switch( state )
		{
		case start :

			currentTime = TIM2->CNT - lastTime;
			/* process period every 100ms = 10Hz */
			vTaskDelay(100 - (currentTime/1000));

			usFlag1 = false;								//set default value
			vTaskPrioritySet(xUS_1, tskIDLE_PRIORITY);		//set priority task and flag to default
			state = ping;									//set next state
			GPIO_ToggleBits(GPIOD, GPIO_Pin_13);			//toggle heart beat ultrasonic task

			lastTime = TIM2->CNT;

	    	break;

		case ping :
			/* start ping signal by pulse 10uSec */
			GPIO_SetBits(GPIOA, GPIO_Pin_6);
			uDelay(10);
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);

			timerState = TIM2->CNT;
			state = wait_risingEcho;

			break;

		case wait_risingEcho :
			if( TIM2->CNT - timerState > 500 ) 				//check TIMEOUT 500us
			{
				/* case none ack form ultrasonic and set status = 1 */
				this_us.status = 1;
				this_us.distance = 0;
				xQueueSend(usQueue, &this_us, 0);
				state = start;								//set state to start state
			}
			else if( usFlag1 != false )
			{
				/* case received rising ack form ultrasonic and time stamp */
				timerState = TIM2->CNT;

				vTaskPrioritySet( xUS_1, tskIDLE_PRIORITY );//set priority task and flag to default
				usFlag1 = false;
				state = wait_fallingEcho;					//set next state
			}
			break;

		case wait_fallingEcho :
			if( TIM2->CNT - timerState > 25000 ) 			//check TIMEOUT 25ms
			{
				/* case distance out of range ack signal more than 25ms */
				this_us.status = 2;
				this_us.distance = 0;
				xQueueSend(usQueue, &this_us, 0);

				/* special case
				 * soft lock interrupt */
				usFlag1 = true;

				state = start;								//set state to start state
			}
			else if( usFlag1 != false )
			{
				/* case success receive signal form ultrasonic  */
				raw = TIM2->CNT - timerState;
				vTaskPrioritySet( xUS_1, tskIDLE_PRIORITY );
				usFlag1 = false;
				state = end;								//set next state
			}
			break;
		case end :
			/* calculate distance */
			distance = ((raw * 17) / 100);
			this_us.status = 0;
			this_us.distance = distance;

			xQueueSend(usQueue, &this_us, 0);				//send data to queue
			state = start;									//set state to start state
			GPIO_ToggleBits(GPIOD, GPIO_Pin_13);			//toggle heart beat ultrasonic task
			break;
		}

    }
}

//task ultrasonic signal process #2
void vTaskUS_2 (void* pvParameters)
{
	/* timer vatriable */
    uint32_t timerState;
    uint32_t currentTime, lastTime = TIM2->CNT;
    /* for calculate distance */
    uint32_t raw, distance;
    /* buffer for send to queue */
    struct us_profile this_us = {2,0,0};
    US_State state = start;

    for (;;)
    {
		switch( state )
		{
		case start :

			currentTime = TIM2->CNT - lastTime;
			/* process period every 100ms = 10Hz */
			vTaskDelay(100 - (currentTime / 1000));

			usFlag2 = false;								//set default value
			vTaskPrioritySet(xUS_2, tskIDLE_PRIORITY);		//set priority task and flag to default
			state = ping;									//set next state

			lastTime = TIM2->CNT;

	    	break;

		case ping :
			/* start ping signal by pulse 10uSec */
			GPIO_SetBits(GPIOA, GPIO_Pin_7);
			uDelay(10);
			GPIO_ResetBits(GPIOA, GPIO_Pin_7);

			timerState = TIM2->CNT;
			state = wait_risingEcho;						//set next state

			break;

		case wait_risingEcho :
			if( TIM2->CNT - timerState > 500 ) 				//check TIMEOUT 500us
			{
				/* case none ack form ultrasonic and set status = 1 */
				this_us.status = 1;
				this_us.distance = 0;
				xQueueSend(usQueue, &this_us, 0);
				state = start;								//set state to start state
			}
			else if( usFlag2 != false )
			{
				/* case received rising ack form ultrasonic and time stamp */
				timerState = TIM2->CNT;
				vTaskPrioritySet( xUS_2, tskIDLE_PRIORITY );//set priority task and flag to default
				usFlag2 = false;
				state = wait_fallingEcho;					//set next state
			}
			break;

		case wait_fallingEcho :
			if( TIM2->CNT - timerState > 25000 ) 			//check TIMEOUT 25ms
			{
				/* case distance out of range ack signal more than 25ms */
				this_us.status = 2;
				this_us.distance = 0;
				xQueueSend(usQueue, &this_us, 0);

				/* special case
				 * soft lock interrupt */
				usFlag2 = true;

				state = start;								//set state to start state
			}
			else if( usFlag2 != false )
			{
				/* case success receive signal form ultrasonic  */
				raw = TIM2->CNT - timerState;
				vTaskPrioritySet( xUS_2, tskIDLE_PRIORITY );//set priority task and flag to default
				usFlag2 = false;
				state = end;								//set next state
			}
			break;
		case end :
			/* calculate distance */
			distance = ((raw * 17) / 100);
			this_us.status = 0;
			this_us.distance = distance;

			xQueueSend(usQueue, &this_us, 0);				//send data to queue
			state = start;									//set state to start state

			break;
		}
    }
}

//task recorder ultrasonic sensor data
void vTaskUS_recorder (void* pvParameters)
{
	struct us_profile us[US_NUMBER];
	struct us_profile us_buffer[US_BUFFER_SIZE] = {0};
	uint8_t i = 0,j = 0;
    for (;;)
    {
    	/* wait data form vTaskUS_(x) */
    	xQueueReceive(usQueue, &us[i], portMAX_DELAY);
    	/* count data to US_NUMBER */
    	i++;
    	if( i == sizeof(us)/sizeof(struct us_profile) )
    	{
    		i = 0;														//reset count value
    		memcpy(&us_buffer[j], us, sizeof(us));						//copy 2 data to internal buffer
    		j += US_NUMBER; 											//count data to US_NUMBER*US_10Hz
    		if( j >= sizeof(us_buffer)/sizeof(struct us_profile) )
    		{
    			j = 0;													//reset count value
    			memcpy(us_data, us_buffer, sizeof(us_buffer));			//copy all data to global buffer

    			/* for testing */
    			//sprintf(str_buffer,"-ID : %u , TYPE : %u , DISTANCE :  %u\r\n",us_data[0].id,us_data[0].status,us_data[0].distance);
    			//USART_puts(USART1, str_buffer);
    		}

    	}
    }
}

//task measure ADC
void vTaskADC (void* pvParameters)
{
	/* timer vatriable */
    uint32_t currentTime, lastTime = TIM2->CNT;
	uint8_t i;

	/* ADC 5CH {id, value} */
    struct adc_profile this_adc[ADC_NUMBER] = { {1,0}, {2,0}, {3,0}, {4,0}, {5,0} };

	for (;;)
	{

		for (i = 0; i < ADC_NUMBER; i++)
		{
			/* ADCValue be DMA */
			this_adc[i].volt = ADCValue[i];

			/* for testing */
			//sprintf(str_buffer,"ADC CH%d : %d\r\n", i, ADCValue[i]);
			//USART_puts(USART1,str_buffer);
		}
		/* send data to queue */
		xQueueSend(adcQueue, &this_adc, 0);

		vTaskDelay(20);
	}
}

//task recorder ADC data
void vTaskADC_recorder (void* pvParameters)
{
	struct adc_profile adc[ADC_NUMBER];
	struct adc_profile adc_buffer[ADC_BUFFER_SIZE] = {0};
	uint8_t j = 0;
    for (;;)
    {
    	xQueueReceive(adcQueue, &adc, portMAX_DELAY);

   		memcpy(&adc_buffer[j], adc, sizeof(adc));
   		j += ADC_NUMBER;											//count data to ADC_BUFFER_SIZE
   		if( j >= sizeof(adc_buffer)/sizeof(struct adc_profile))
   		{
   			j = 0;													//reset count value
   			memcpy(adc_data, adc_buffer, sizeof(adc_buffer));		//copy internal buffer to global buffer

   		}
    }
}

//task capture PWM
void vTaskROS (void* pvParameters)
{
	uint8_t i;
    uint32_t currentTime, lastTime = TIM2->CNT;
    /* Reflective 4CH {id, value} */
    struct ros_profile this_ros[ROS_NUMBER] = { {1,0}, {2,0}, {3,0}, {4,0} };

    for(;;)
	{
		for (i = 0; i < ROS_NUMBER; i++) {
			this_ros[i].speed = roundTime[i];

			/* for testing */
			//sprintf(str_buffer, "PWM CH%d : %u\r\n", i, (unsigned int) roundTime[i]);
			//USART_puts(USART1, str_buffer);
		}
		//send 4 data to queue
		xQueueSend(rosQueue, &this_ros, 0);
		//Enable external interrupt
		enableExtiROS();

		//frequency 10Hz
		vTaskDelay(100);
	}
}

//task recorder Reflective optical sensor data
void vTaskROS_recorder (void* pvParameters)
{
	struct ros_profile ros[ROS_NUMBER];
	struct ros_profile ros_buffer[ROS_BUFFER_SIZE] = {0};
	uint8_t j = 0;
    for (;;)
    {
    	/* wait data form vTaskROS */
    	xQueueReceive(rosQueue, &ros, portMAX_DELAY);
    	memcpy(&ros_buffer[j], ros, sizeof(ros));				//copy 4 data to internal buffer
    	j += ROS_NUMBER;										//count data to ROS_BUFFER_SIZE
    	if( j >= sizeof(ros_buffer)/sizeof(struct ros_profile))
    	{
    		j = 0;												//reset count value
    		memcpy(ros_data, ros_buffer, sizeof(ros_buffer));	//copy internal buffer to global buffer

    		/* for testing */
			//sprintf(str_buffer,"i %u, s %u\r\n",ros_data[0].id,ros_data[0].speed);
			//USART_puts(USART1, str_buffer);
    	}
    }
}

//task sender data to computer
void vTaskSendData (void* pvParameters)
{
	uint32_t currentTime, lastTime = TIM2->CNT;
	uint16_t i;
	vTaskDelay(2000);

    for (;;)
    {

		/* send all data to computer
		 * us_data
		 * ros_data
		 * adc_data
		 */

    	lastTime = TIM2->CNT;

		USART_puts(USART1, "Ultrasonic Data for 1 Second\r\n");
		for(i = 0; i < (sizeof(us_data)/sizeof(struct us_profile)); i++ )
		{
			sprintf(str_buffer,"i %u, s %u, d %u\r\n",us_data[i].id,us_data[i].status,us_data[i].distance);
			USART_puts(USART1, str_buffer);
		}

		USART_puts(USART1, "ADC Data for 1 Second\r\n");
		for(i = 0; i < (sizeof(adc_data)/sizeof(struct adc_profile)); i+=5 )
		{
			sprintf(str_buffer,"i %u, v %u, i %u, v %u, i %u, v %u, i %u, v %u, i %u, v %u\r\n"
														  ,adc_data[i].id,adc_data[i].volt
			                                              ,adc_data[i+1].id,adc_data[i+1].volt
			             			                      ,adc_data[i+2].id,adc_data[i+2].volt
			             			                      ,adc_data[i+3].id,adc_data[i+3].volt
			             			             		  ,adc_data[i+4].id,adc_data[i+4].volt
			);
			USART_puts(USART1, str_buffer);
		}

		USART_puts(USART1, "Reflective Optical Data for 1 Second\r\n");
		for(i = 0; i < (sizeof(ros_data)/sizeof(struct ros_profile)); i++ )
		{
			sprintf(str_buffer,"i %u, s %u\r\n",ros_data[i].id,ros_data[i].speed);
			USART_puts(USART1, str_buffer);
		}

		currentTime = TIM2->CNT - lastTime;

		/* for testing */
		//sprintf(str_buffer,"%d %d\r\n",1000 - (currentTime/1000),(currentTime/1000));
		//USART_puts(USART1, str_buffer);

		vTaskDelay(1000 - (currentTime/1000));

    }
}

int main(void)
{

	GPIO_init();
	USART_init();
	Timer_init();
	EXTI_init();
	ADC_init();

	USART_puts(USART1, "Project v2.0 Started.\r\n");

	usQueue  = xQueueCreate( 10, sizeof(struct us_profile ));
	rosQueue = xQueueCreate( 20, sizeof(struct ros_profile)*ROS_NUMBER);
	adcQueue = xQueueCreate( 25, sizeof(struct adc_profile)*ADC_NUMBER);

	xTaskCreate(vTaskBlink, 		"Heartbeat", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskUS_1, 			"Ultrasonic#1", configMINIMAL_STACK_SIZE, NULL, 1, &xUS_1);
	xTaskCreate(vTaskUS_2, 			"Ultrasonic#2", configMINIMAL_STACK_SIZE, NULL, 1, &xUS_2);
	xTaskCreate(vTaskUS_recorder, 	"Ultrasonic recorder", configMINIMAL_STACK_SIZE+128, NULL, 1, NULL);
	xTaskCreate(vTaskADC, 			"ADC", configMINIMAL_STACK_SIZE+512, NULL, 1, NULL);
	xTaskCreate(vTaskADC_recorder, 	"ADC recorder", configMINIMAL_STACK_SIZE+512, NULL, 1, NULL);
	xTaskCreate(vTaskROS, 			"Reflective optical sensor", configMINIMAL_STACK_SIZE+128, NULL, 1, NULL);
	xTaskCreate(vTaskROS_recorder, 	"Reflective optical recorder", configMINIMAL_STACK_SIZE+128, NULL, 1, NULL);
	xTaskCreate(vTaskSendData, 		"Send all data", configMINIMAL_STACK_SIZE+512, NULL, 1, NULL);

	vTaskStartScheduler();

	for(;;);

	return 0;
}

void EXTI0_IRQHandler(void) {
	if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
		/* Toggle LED */
		//GPIO_ToggleBits(GPIOD, GPIO_Pin_12 |GPIO_Pin_13);
		if (roundFlag[0] == 0) {
			roundFlag[0] = 1;

			roundLastTime[0] = TIM2->CNT;
		} else {
			roundFlag[0] = 0;

			roundTime[0] = TIM2->CNT - roundLastTime[0];

			EXTI_InitTypeDef EXTI_InitStructure;
			EXTI_InitStructure.EXTI_Line = EXTI_Line0;
			EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
			EXTI_InitStructure.EXTI_LineCmd = DISABLE;
			EXTI_Init(&EXTI_InitStructure);
		}

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

void EXTI1_IRQHandler(void) {
	if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
		if (roundFlag[1] == 0) {
			roundFlag[1] = 1;

			roundLastTime[1] = TIM2->CNT;
		} else {
			roundFlag[1] = 0;

			roundTime[1] = TIM2->CNT - roundLastTime[1];

			EXTI_InitTypeDef EXTI_InitStructure;
			EXTI_InitStructure.EXTI_Line = EXTI_Line1;
			EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
			EXTI_InitStructure.EXTI_LineCmd = DISABLE;
			EXTI_Init(&EXTI_InitStructure);
		}

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}

void EXTI2_IRQHandler(void) {
	if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
		if (roundFlag[2] == 0) {
			roundFlag[2] = 1;

			roundLastTime[2] = TIM2->CNT;
		} else {
			roundFlag[2] = 0;

			roundTime[2] = TIM2->CNT - roundLastTime[2];

			EXTI_InitTypeDef EXTI_InitStructure;
			EXTI_InitStructure.EXTI_Line = EXTI_Line2;
			EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
			EXTI_InitStructure.EXTI_LineCmd = DISABLE;
			EXTI_Init(&EXTI_InitStructure);
		}

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line2);
	}
}

void EXTI3_IRQHandler(void) {
	if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
		if (roundFlag[3] == 0) {
			roundFlag[3] = 1;

			roundLastTime[3] = TIM2->CNT;
		} else {
			roundFlag[3] = 0;

			roundTime[3] = TIM2->CNT - roundLastTime[3];

			EXTI_InitTypeDef EXTI_InitStructure;
			EXTI_InitStructure.EXTI_Line = EXTI_Line3;
			EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
			EXTI_InitStructure.EXTI_LineCmd = DISABLE;
			EXTI_Init(&EXTI_InitStructure);
		}

		/* Clear the EXTI line 0 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line3);
	}
}

void EXTI15_10_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line10) != RESET)
  {
	  /* Clear the EXTI line 0 pending bit */
	  EXTI_ClearITPendingBit(EXTI_Line10);
	  if( usFlag1 == false ){
		  vTaskPrioritySet( xUS_1, tskIDLE_PRIORITY + 1 );
		  usFlag1 = true;
	  }
  }

  else if(EXTI_GetITStatus(EXTI_Line15) != RESET)
  {
	  /* Clear the EXTI line 15 pending bit */
	  EXTI_ClearITPendingBit(EXTI_Line15);
	  if( usFlag2 == false ){
		  vTaskPrioritySet( xUS_2, tskIDLE_PRIORITY + 1 );
		  usFlag2 = true;
	 }
  }
}

void USART1_IRQHandler(void){

  if( USART_GetITStatus(USART1, USART_IT_RXNE) )
  {
    char c = USART1->DR;
    USART_puts(USART1, &c);
  }
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	taskDISABLE_INTERRUPTS();
	for(;;);
}
void vApplicationMallocFailedHook(void) {
	taskDISABLE_INTERRUPTS();
	for(;;);
}
void vApplicationTickHook(void) {}
void vApplicationIdleHook(void) {}
