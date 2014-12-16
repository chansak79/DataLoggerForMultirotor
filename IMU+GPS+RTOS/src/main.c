#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_i2c.h"
#include "MPU6050.h"
#include "HMC5883L.h"
#include "MS5611.h"
#include "misc.h"
#include <stdio.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "defines.h"
#include "tm_stm32f4_fatfs.h"

#define GPS_BUFFER_SIZE 100
#define IMU_BUFFER_SIZE 100

typedef enum { false, true } bool;

void Timer_init			(void);
void GPIO_init			(void);
void SDcard_start		(bool* mount_OK);
void USART_init			(void);
void USART_puts			(USART_TypeDef* USARTx, volatile char *c);
void ringBufferInit		(uint8_t length);
void ringBufferWrite	(char *data);
void ringBufferRead		(char *data);
void uDelay				(uint32_t uS);
void GY86_I2C_Init		(void);
void RTC_Config			(void);
void ButtonInit			(void);
uint32_t GetLSIFrequency(void);

xQueueHandle gpsQueue;
xQueueHandle gpsRecordQueue;
xQueueHandle imuRecordQueue;

char receivestring[GPS_BUFFER_SIZE];

FATFS FatFs;					//Fatfs object
FIL fil[2],temp_fil[2];				//File object
char *file_name[] = {"gps.txt",'imu.txt'};
uint16_t * fc;

void vTaskBlink (void* pvParameters)
{
	for (;;)
	{
		uDelay(500000);
		GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
	}
}


void vTaskSDcard (void* pvParameters)
{
	char gps_data[GPS_BUFFER_SIZE];
	char imu_data[IMU_BUFFER_SIZE];
	uint8_t i;

	uint32_t currentTime = TIM2->CNT, lastTime;
	for(;;){
		lastTime = TIM2->CNT;
		//Record GPS
		f_open(&fil[0], file_name[0], FA_OPEN_ALWAYS | FA_WRITE);
		fil[0] = temp_fil[0];
		i = 10;
		while(i > 0){
			if(!xQueueIsQueueEmptyFromISR(gpsRecordQueue)){
				xQueueReceive(gpsRecordQueue, &gps_data, 0);
				f_puts(gps_data, &fil[0]);
				gps_data[0] = '\0';
				i--;
			}
			else i = 0;
		}
		temp_fil[0] = fil[0];
		f_close(&fil[0]);

		//Record IMU
		f_open(&fil[1], file_name[1], FA_OPEN_ALWAYS | FA_WRITE);
		fil[1] = temp_fil[1];
		i = 50;
		while(i > 0){
			if(!xQueueIsQueueEmptyFromISR(imuRecordQueue)){
				xQueueReceive(imuRecordQueue, &imu_data, 0);
				f_puts(imu_data, &fil[1]);
				imu_data[0] = '\0';
				i--;
			}
		}
		temp_fil[1] = fil[1];
		f_close(&fil[1]);

		currentTime = TIM2->CNT - lastTime;
		vTaskDelay(1000 - (currentTime/1000));
	}
}

void vTaskGPS (void* pvParameters){
	char gps_data[GPS_BUFFER_SIZE];
	char receive_data[GPS_BUFFER_SIZE];
	char NMEA_protocol[6] = "$GPGGA";
	int NMEA_protocol_size = sizeof(NMEA_protocol)/sizeof(char);
	int i, data_size;
	for (;;)
	{
		xQueueReceive(gpsQueue, &receive_data, portMAX_DELAY);
		data_size = sizeof(receive_data)/sizeof(char);
		for(i = 0;i < NMEA_protocol_size;i++){
			if (receive_data[i] != NMEA_protocol[i]){
				//char s[] = "";
				//sprintf(s,"%c\n\r",receive_data);
				USART_puts(USART1, receive_data);
				data_size = 0;
				break;
			}
		}

		if (receive_data[NMEA_protocol_size + 1] == ','){
			int count_comma = 0;
			int j = 0;
			for(i = NMEA_protocol_size + 2;i < data_size;i++){
				if (receive_data[i] == ','){
					count_comma++;
					if (count_comma == 5){
						break;
					}
				}
				gps_data[j] = receive_data[i];
				j++;
			}
			gps_data[j+1] = '\0';
			xQueueSend(gpsRecordQueue, &gps_data, 0);
			gps_data[0] = '\0';
		}

	}
}

void vTaskReadIMU (void* pvParameters)
{
	int16_t  AccelGyro[6]={0};
	int16_t  Compass[3]={0};
	uint32_t  rawPressure = 0;
	uint32_t  rawTemperature = 0;
	int32_t  Pres_Temp[2] = {0};
	int32_t  time_stamp = 0;

	for(;;){
		// Read raw accelerometer and gyroscope from MPU6050.
		MPU6050_GetRawAccelGyro(AccelGyro);
		// Read raw compass from HMC5883L.
		HMC5883L_GetHeading(Compass);
		// Read raw pressure from MS5611.
		rawPressure = MS5611_GetRawPressure();
		// Read raw temperature from MS5611.
		rawTemperature = MS5611_GetRawTemperature();
		// Calculate for temperature and pressure.
		MS5611_Calculate(fc, rawTemperature, rawPressure, Pres_Temp);

		char s[] = "";
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);

		sprintf(s, "%d.%d.%d,%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,%ld\n\r", RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds,time_stamp
				, AccelGyro[0], AccelGyro[1], AccelGyro[2], AccelGyro[3], AccelGyro[4], AccelGyro[5]
				, Compass[0], Compass[1], Compass[2], Pres_Temp[0], Pres_Temp[1]);

		xQueueSend(imuRecordQueue, &s, 0);
	}
}

int main(void)
{
	bool mount_OK = false;
	GPIO_init();

	ButtonInit();
	/* RTC Configuration -------------------------------------------------------*/
	RTC_Config();

	/* Wait Until KEY BUTTON is pressed */
	while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == RESET)
	{
	}

	/* Get the LSI frequency:  TIM5 is used to measure the LSI frequency */
	LsiFreq = GetLSIFrequency();

	/* RTC Configuration */
	RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
	RTC_InitStructure.RTC_SynchPrediv	= (LsiFreq/128) - 1;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_Init(&RTC_InitStructure);

	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_TimeStructure.RTC_Hours = 0;
	RTC_TimeStructure.RTC_Minutes = 0;
	RTC_TimeStructure.RTC_Seconds = 0;
	RTC_SetTime(RTC_Format_BIN,&RTC_TimeStructure);

	RTC_DateTypeDef RTC_DateStructure;
	RTC_DateStructure.RTC_Date = 12;
	RTC_DateStructure.RTC_Month = 11;
	RTC_DateStructure.RTC_Year = 14;
	RTC_SetDate(RTC_Format_BIN,&RTC_DateStructure);

	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	uint8_t temp = RTC_TimeStructure.RTC_Seconds;

	Timer_init();
	GY86_I2C_Init();
	while(mount_OK == false)
	{
		SDcard_start(&mount_OK);
		GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
		uDelay(500000);
	}
	USART_init();
	// MPU6050 initialize.
	MPU6050_Initialize();
	// Enable I2C bypass mode.
	MPU6050_WriteBit(MPU6050_DEFAULT_ADDRESS, MPU6050_RA_INT_PIN_CFG, MPU6050_INTCFG_I2C_BYPASS_EN_BIT, 1);
	// HMC5883L initialize.
	HMC5883L_Initialize();
	// MS5611 initialize.
	MS5611_Initialize();

	fc = MS5611_readPROM();

	gpsQueue  = xQueueCreate( 50, sizeof(char[100])/sizeof(char));
	gpsRecordQueue  = xQueueCreate( 50, sizeof(char[100])/sizeof(char));
	imuRecordQueue  = xQueueCreate( 100, sizeof(char[100])/sizeof(char));

	xTaskCreate(vTaskBlink, "Heartbeat", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(vTaskSDcard, "SDcard", configMINIMAL_STACK_SIZE+512, NULL, 1, NULL);
	xTaskCreate(vTaskGPS, "GPS", configMINIMAL_STACK_SIZE+128, NULL, 1, NULL);
	xTaskCreate(vTaskReadIMU, "ReadIMU", configMINIMAL_STACK_SIZE+128, NULL, 1, NULL);

	vTaskStartScheduler();
	for(;;);

	return 0;
}

void SDcard_start(bool* mount_OK)
{
     if(f_mount(&FatFs, "", 1) == FR_OK)
     {
    	 GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
    	 int i;
    	 for(i = 0;i < 2;i++){
			 if(f_open(&fil[i], file_name[i], FA_OPEN_ALWAYS | FA_WRITE) == FR_OK)
			 {
				 GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
				 temp_fil = fil;
				 GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
				 f_close(&fil);
				 *mount_OK = true; // mount is OK
			 }
			 else *mount_OK = false; // mount isn't OK
    	 }

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

void USART1_IRQHandler(void){
	static uint8_t cnt = 0;
	if( USART_GetITStatus(USART1, USART_IT_RXNE) )
	{
		char t = USART1->DR; // the character from the USART1 data register is saved in t
		if(t == '$'){
			receivestring[cnt] = '\0';
			xQueueSend(gpsQueue, &receivestring, 0);
			cnt = 0;
		}
		receivestring[cnt] = t;
		cnt++;

	}
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
}

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

void GY86_I2C_Init()
{
  I2C_InitTypeDef I2C_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable I2C and GPIO clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  /* Configure I2C pins: SCL and SDA */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // I2C2_SCL and I2C2_SDA pins connect to their AF
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

  /* I2C configuration */
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 400000;

  /* I2C Peripheral Enable */
  I2C_Cmd(I2C2, ENABLE);

  /* Apply I2C configuration after enabling it */
  I2C_Init(I2C2, &I2C_InitStructure);

}

void RTC_Config(void)
{
	/* Enable the PWR clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to RTC */
	PWR_BackupAccessCmd(ENABLE);

	/* LSI used as RTC source clock */
	/* The RTC Clock may varies due to LSI frequency dispersion. */
	/* Enable the LSI OSC */
	RCC_LSICmd(ENABLE);

	/* Wait till LSI is ready */
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	{
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronisation */
	RTC_WaitForSynchro();
}

uint32_t GetLSIFrequency(void)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_ICInitTypeDef  TIM_ICInitStructure;
	RCC_ClocksTypeDef  RCC_ClockFreq;

	/* Enable the LSI oscillator ************************************************/
	RCC_LSICmd(ENABLE);

	/* Wait till LSI is ready */
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	{}

	/* TIM5 configuration *******************************************************/
	/* Enable TIM5 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	/* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
	TIM_RemapConfig(TIM5, TIM5_LSI);

	/* Configure TIM5 presclaer */
	TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

	/* TIM5 configuration: Input Capture mode ---------------------
     	 The LSI oscillator is connected to TIM5 CH4
     	 The Rising edge is used as active edge,
     	 The TIM5 CCR4 is used to compute the frequency value
  	  ------------------------------------------------------------ */
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
	TIM_ICInitStructure.TIM_ICFilter = 0;
	TIM_ICInit(TIM5, &TIM_ICInitStructure);

	/* Enable TIM5 Interrupt channel */
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable TIM5 counter */
	TIM_Cmd(TIM5, ENABLE);

	/* Reset the flags */
	TIM5->SR = 0;

	/* Enable the CC4 Interrupt Request */
	TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);


	/* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler() in
    	stm32f4xx_it.c file) ******************************************************/
	while(CaptureNumber != 2)
	{
	}
	/* Deinitialize the TIM5 peripheral registers to their default reset values */
	TIM_DeInit(TIM5);


	/* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1)*/
	/* Get SYSCLK, HCLK and PCLKx frequency */
	RCC_GetClocksFreq(&RCC_ClockFreq);

	/* Get PCLK1 prescaler */
	if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
	{
		/* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
		return ((RCC_ClockFreq.PCLK1_Frequency / PeriodValue) * 8);
	}
	else
	{ /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
		return (((2 * RCC_ClockFreq.PCLK1_Frequency) / PeriodValue) * 8) ;
	}
}

void ButtonInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable the BUTTON Clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Configure Button pin as input */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
  * @brief  This function handles TIM5 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM5_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM5, TIM_IT_CC4) != RESET)
  {
    /* Get the Input Capture value */
    tmpCC4[CaptureNumber++] = TIM_GetCapture4(TIM5);

    /* Clear CC4 Interrupt pending bit */
    TIM_ClearITPendingBit(TIM5, TIM_IT_CC4);

    if (CaptureNumber >= 2)
    {
      /* Compute the period length */
      PeriodValue = (uint16_t)(0xFFFF - tmpCC4[0] + tmpCC4[1] + 1);
    }
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
  USART_InitStructure.USART_BaudRate              = 9600;
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

void uDelay(uint32_t uS)
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


