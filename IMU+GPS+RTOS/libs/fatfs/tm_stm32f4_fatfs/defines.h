/**
 *  Defines for your entire project at one place
 * 
 *	@author 	Tilen Majerle
 *	@email		tilen@majerle.eu
 *	@website	http://stm32f4-discovery.com
 *	@version 	v1.0
 *	@ide		Keil uVision 5
 */
#ifndef TM_DEFINES_H
#define TM_DEFINES_H

//Use SPI communication with SDCard
#define	FATFS_USE_SDIO		0

//Select your SPI settings
#define FATFS_SPI		SPI1
#define FATFS_SPI_PINSPACK	TM_SPI_PinsPack_1

//Custom CS pin for SPI communication
#define FATFS_CS_RCC		RCC_AHB1Periph_GPIOC
#define FATFS_CS_PORT		GPIOC
#define FATFS_CS_PIN		GPIO_Pin_4
//Put your global defines for all libraries here used in your project

#endif
