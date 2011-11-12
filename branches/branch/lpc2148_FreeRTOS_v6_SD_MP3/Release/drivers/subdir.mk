################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/i2c.c \
../drivers/pcm1774.c \
../drivers/ssp_spi.c \
../drivers/sta013.c 

OBJS += \
./drivers/i2c.o \
./drivers/pcm1774.o \
./drivers/ssp_spi.o \
./drivers/sta013.o 

C_DEPS += \
./drivers/i2c.d \
./drivers/pcm1774.d \
./drivers/ssp_spi.d \
./drivers/sta013.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-elf-gcc -Os -Wall -std=gnu99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi-s -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


