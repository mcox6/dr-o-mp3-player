################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../System/cpu.c \
../System/crash.c \
../System/fiqISR.c \
../System/rprintf.c \
../System/syscalls.c \
../System/watchdog.c 

OBJS += \
./System/cpu.o \
./System/crash.o \
./System/fiqISR.o \
./System/rprintf.o \
./System/syscalls.o \
./System/watchdog.o 

C_DEPS += \
./System/cpu.d \
./System/crash.d \
./System/fiqISR.d \
./System/rprintf.d \
./System/syscalls.d \
./System/watchdog.d 


# Each subdirectory must supply rules for building sources it contributes
System/%.o: ../System/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-elf-gcc -O0 -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=cortex-m3 -mthumb -g3 -gdwarf-2 -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


