################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libs/FreeRTOS/portable/heap_2.c \
../libs/FreeRTOS/portable/port.c 

OBJS += \
./libs/FreeRTOS/portable/heap_2.o \
./libs/FreeRTOS/portable/port.o 

C_DEPS += \
./libs/FreeRTOS/portable/heap_2.d \
./libs/FreeRTOS/portable/port.d 


# Each subdirectory must supply rules for building sources it contributes
libs/FreeRTOS/portable/%.o: ../libs/FreeRTOS/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -DDEBUG -I"../libs/misc/inc" -I"../inc" -I"../libs/cmsis/inc" -I"../libs/StdPeriph/inc" -I"/home/chansak/workspace/Project_v2.0/libs/FreeRTOS/inc" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


