################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/custom_driver/mcal/isr_bridge.cpp 

OBJS += \
./Drivers/custom_driver/mcal/isr_bridge.o 

CPP_DEPS += \
./Drivers/custom_driver/mcal/isr_bridge.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/custom_driver/mcal/%.o Drivers/custom_driver/mcal/%.su Drivers/custom_driver/mcal/%.cyclo: ../Drivers/custom_driver/mcal/%.cpp Drivers/custom_driver/mcal/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-custom_driver-2f-mcal

clean-Drivers-2f-custom_driver-2f-mcal:
	-$(RM) ./Drivers/custom_driver/mcal/isr_bridge.cyclo ./Drivers/custom_driver/mcal/isr_bridge.d ./Drivers/custom_driver/mcal/isr_bridge.o ./Drivers/custom_driver/mcal/isr_bridge.su

.PHONY: clean-Drivers-2f-custom_driver-2f-mcal

