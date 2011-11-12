################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/uart/uart0.c 

OBJS += \
./drivers/uart/uart0.o 

C_DEPS += \
./drivers/uart/uart0.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/uart/%.o: ../drivers/uart/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-elf-gcc -Os -Wall -std=gnu99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi-s -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


