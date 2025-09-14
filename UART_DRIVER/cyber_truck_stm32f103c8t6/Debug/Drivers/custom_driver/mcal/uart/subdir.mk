################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/custom_driver/mcal/uart/uart.cpp 

OBJS += \
./Drivers/custom_driver/mcal/uart/uart.o 

CPP_DEPS += \
./Drivers/custom_driver/mcal/uart/uart.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/custom_driver/mcal/uart/%.o Drivers/custom_driver/mcal/uart/%.su Drivers/custom_driver/mcal/uart/%.cyclo: ../Drivers/custom_driver/mcal/uart/%.cpp Drivers/custom_driver/mcal/uart/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-custom_driver-2f-mcal-2f-uart

clean-Drivers-2f-custom_driver-2f-mcal-2f-uart:
	-$(RM) ./Drivers/custom_driver/mcal/uart/uart.cyclo ./Drivers/custom_driver/mcal/uart/uart.d ./Drivers/custom_driver/mcal/uart/uart.o ./Drivers/custom_driver/mcal/uart/uart.su

.PHONY: clean-Drivers-2f-custom_driver-2f-mcal-2f-uart

