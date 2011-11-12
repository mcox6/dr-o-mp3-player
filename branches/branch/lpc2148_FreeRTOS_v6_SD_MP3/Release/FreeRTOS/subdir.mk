################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/croutine.c \
../FreeRTOS/heap_3.c \
../FreeRTOS/list.c \
../FreeRTOS/port.c \
../FreeRTOS/portISR.c \
../FreeRTOS/queue.c \
../FreeRTOS/tasks.c 

OBJS += \
./FreeRTOS/croutine.o \
./FreeRTOS/heap_3.o \
./FreeRTOS/list.o \
./FreeRTOS/port.o \
./FreeRTOS/portISR.o \
./FreeRTOS/queue.o \
./FreeRTOS/tasks.o 

C_DEPS += \
./FreeRTOS/croutine.d \
./FreeRTOS/heap_3.d \
./FreeRTOS/list.d \
./FreeRTOS/port.d \
./FreeRTOS/portISR.d \
./FreeRTOS/queue.d \
./FreeRTOS/tasks.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/%.o: ../FreeRTOS/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Windows GCC C Compiler'
	arm-elf-gcc -Os -Wall -std=gnu99 -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi-s -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


