################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include fat/subdir.mk
-include fat/option/subdir.mk
-include drivers/uart/subdir.mk
-include drivers/subdir.mk
-include System/subdir.mk
-include General/subdir.mk
-include FreeRTOS/subdir.mk
-include FreeRTOS/hooks/subdir.mk
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
lpc2148_FreeRTOS_v6_4.hex \

SECONDARY_LIST += \
lpc2148_FreeRTOS_v6_4.lst \

SECONDARY_SIZE += \
lpc2148_FreeRTOS_v6_4.siz \


# All Target
all: lpc2148_FreeRTOS_v6_4.elf secondary-outputs

# Tool invocations
lpc2148_FreeRTOS_v6_4.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Windows GCC C Linker'
	arm-elf-gcc -T"..\lpc2148-rom.ld" -nostartfiles -Wl,-Map,lpc2148_FreeRTOS_v6_4.map -mcpu=arm7tdmi-s -o"lpc2148_FreeRTOS_v6_4.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

lpc2148_FreeRTOS_v6_4.hex: lpc2148_FreeRTOS_v6_4.elf
	@echo 'Invoking: ARM Windows GNU Create Flash Image'
	arm-elf-objcopy -O ihex lpc2148_FreeRTOS_v6_4.elf "lpc2148_FreeRTOS_v6_4.hex"
	@echo 'Finished building: $@'
	@echo ' '

lpc2148_FreeRTOS_v6_4.lst: lpc2148_FreeRTOS_v6_4.elf
	@echo 'Invoking: ARM Windows GNU Create Listing'
	arm-elf-objdump -h -S lpc2148_FreeRTOS_v6_4.elf >"lpc2148_FreeRTOS_v6_4.lst"
	@echo 'Finished building: $@'
	@echo ' '

lpc2148_FreeRTOS_v6_4.siz: lpc2148_FreeRTOS_v6_4.elf
	@echo 'Invoking: ARM Windows GNU Print Size'
	arm-elf-size  --format=berkeley lpc2148_FreeRTOS_v6_4.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) lpc2148_FreeRTOS_v6_4.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
