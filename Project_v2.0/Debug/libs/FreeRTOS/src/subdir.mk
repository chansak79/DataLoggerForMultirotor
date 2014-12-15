################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libs/FreeRTOS/src/croutine.c \
../libs/FreeRTOS/src/event_groups.c \
../libs/FreeRTOS/src/list.c \
../libs/FreeRTOS/src/queue.c \
../libs/FreeRTOS/src/tasks.c \
../libs/FreeRTOS/src/timers.c 

OBJS += \
./libs/FreeRTOS/src/croutine.o \
./libs/FreeRTOS/src/event_groups.o \
./libs/FreeRTOS/src/list.o \
./libs/FreeRTOS/src/queue.o \
./libs/FreeRTOS/src/tasks.o \
./libs/FreeRTOS/src/timers.o 

C_DEPS += \
./libs/FreeRTOS/src/croutine.d \
./libs/FreeRTOS/src/event_groups.d \
./libs/FreeRTOS/src/list.d \
./libs/FreeRTOS/src/queue.d \
./libs/FreeRTOS/src/tasks.d \
./libs/FreeRTOS/src/timers.d 


# Each subdirectory must supply rules for building sources it contributes
libs/FreeRTOS/src/%.o: ../libs/FreeRTOS/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -DDEBUG -I"../libs/misc/inc" -I"../inc" -I"../libs/cmsis/inc" -I"../libs/StdPeriph/inc" -I"/home/chansak/workspace/Project_v2.0/libs/FreeRTOS/inc" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


