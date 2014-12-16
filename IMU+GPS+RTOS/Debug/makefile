################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include libs/misc/src/subdir.mk
-include libs/fatfs/tm_stm32f4_spi/subdir.mk
-include libs/fatfs/tm_stm32f4_fatfs/fatfs/option/subdir.mk
-include libs/fatfs/tm_stm32f4_fatfs/fatfs/drivers/subdir.mk
-include libs/fatfs/tm_stm32f4_fatfs/fatfs/subdir.mk
-include libs/fatfs/tm_stm32f4_fatfs/subdir.mk
-include libs/fatfs/tm_stm32f4_delay/subdir.mk
-include libs/cmsis/src/subdir.mk
-include libs/StdPeriph/src/subdir.mk
-include libs/STM32_MPU6050lib-master/subdir.mk
-include libs/STM32_HMC5883Llib-master/subdir.mk
-include libs/FreeRTOS/src/subdir.mk
-include libs/FreeRTOS/portable/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
IMU+GPS+RTOS.bin \

SECONDARY_LIST += \
IMU+GPS+RTOS.lst \

SECONDARY_SIZE += \
IMU+GPS+RTOS.siz \


# All Target
all: IMU+GPS+RTOS.elf secondary-outputs

# Tool invocations
IMU+GPS+RTOS.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall  -g3 -T libs.ld -T mem.ld -T sections.ld -nostartfiles -Xlinker --gc-sections -L"../ldscripts" -Wl,-Map,"IMU+GPS+RTOS.map" -o "IMU+GPS+RTOS.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

IMU+GPS+RTOS.bin: IMU+GPS+RTOS.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy "IMU+GPS+RTOS.elf" -O binary  "IMU+GPS+RTOS.bin"
	@echo 'Finished building: $@'
	@echo ' '

IMU+GPS+RTOS.lst: IMU+GPS+RTOS.elf
	@echo 'Invoking: Cross ARM GNU Create Listing'
	arm-none-eabi-objdump "IMU+GPS+RTOS.elf" --source --all-headers --demangle --line-numbers --wide > "IMU+GPS+RTOS.lst"
	@echo 'Finished building: $@'
	@echo ' '

IMU+GPS+RTOS.siz: IMU+GPS+RTOS.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size  --format=berkeley "IMU+GPS+RTOS.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(S_UPPER_DEPS) IMU+GPS+RTOS.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
