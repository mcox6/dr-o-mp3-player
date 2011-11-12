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
MP3v3.hex \

SECONDARY_LIST += \
MP3v3.lst \

SECONDARY_SIZE += \
MP3v3.siz \


# All Target
all: MP3v3.elf secondary-outputs

# Tool invocations
MP3v3.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Windows GCC C Linker'
	arm-elf-gcc -nostartfiles -Wl,-Map,MP3v3.map -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o"MP3v3.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

MP3v3.hex: MP3v3.elf
	@echo 'Invoking: ARM Windows GNU Create Flash Image'
	arm-elf-objcopy -O ihex MP3v3.elf "MP3v3.hex"
	@echo 'Finished building: $@'
	@echo ' '

MP3v3.lst: MP3v3.elf
	@echo 'Invoking: ARM Windows GNU Create Listing'
	arm-elf-objdump -h -S MP3v3.elf >"MP3v3.lst"
	@echo 'Finished building: $@'
	@echo ' '

MP3v3.siz: MP3v3.elf
	@echo 'Invoking: ARM Windows GNU Print Size'
	arm-elf-size  --format=berkeley MP3v3.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(EXECUTABLES)$(SECONDARY_LIST)$(S_UPPER_DEPS) MP3v3.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
