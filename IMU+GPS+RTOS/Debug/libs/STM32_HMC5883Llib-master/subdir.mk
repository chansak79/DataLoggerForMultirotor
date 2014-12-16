################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libs/STM32_HMC5883Llib-master/HMC5883L.c 

OBJS += \
./libs/STM32_HMC5883Llib-master/HMC5883L.o 

C_DEPS += \
./libs/STM32_HMC5883Llib-master/HMC5883L.d 


# Each subdirectory must supply rules for building sources it contributes
libs/STM32_HMC5883Llib-master/%.o: ../libs/STM32_HMC5883Llib-master/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -DDEBUG -I"../libs/misc/include" -I"../include" -I"../libs/cmsis/include" -I"../libs/StdPeriph/include" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/fatfs" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/FreeRTOS" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/STM32_HMC5883Llib-master" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/STM32_MPU6050lib-master" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/FreeRTOS/inc" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/fatfs/tm_stm32f4_fatfs" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/fatfs/tm_stm32f4_fatfs/fatfs" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/fatfs/tm_stm32f4_fatfs/fatfs/drivers" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/fatfs/tm_stm32f4_spi" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/fatfs/tm_stm32f4_delay" -I"/home/paint20/workspace/IMU+GPS+RTOS/libs/MS5611lib" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


