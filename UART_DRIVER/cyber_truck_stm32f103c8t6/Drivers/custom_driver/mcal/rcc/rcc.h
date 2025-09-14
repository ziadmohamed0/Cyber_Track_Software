/*
 * rcc.h
 *
 *  Created on: Sep 3, 2025
 *      Author: ziad
 */

#ifndef CUSTOM_DRIVER_MCAL_RCC_RCC_H_
#define CUSTOM_DRIVER_MCAL_RCC_RCC_H_

#include "../../lib/common.h"

/* APB2 peripherals */
constexpr uint32_t clock_afio = 0;
constexpr uint32_t clock_gpio_a = 2;
constexpr uint32_t clock_gpio_b = 3;
constexpr uint32_t clock_gpio_c = 4;
constexpr uint32_t clock_uart_1 = 14;
constexpr uint32_t clock_adc_1 = 9;
constexpr uint32_t clock_adc_2 = 10;
constexpr uint32_t clock_timer_1 = 11;
constexpr uint32_t clock_spi_1 = 12;

/* APB1 peripherals */
constexpr uint32_t clock_timer_2 = 0;
constexpr uint32_t clock_timer_3 = 1;
constexpr uint32_t clock_timer_4 = 2;
constexpr uint32_t clock_timer_5 = 3;
constexpr uint32_t clock_timer_6 = 4;
constexpr uint32_t clock_timer_7 = 5;
constexpr uint32_t clock_uart_2 = 17;
constexpr uint32_t clock_uart_3 = 18;
constexpr uint32_t clock_uart_4 = 19;
constexpr uint32_t clock_uart_5 = 20;

enum class periphrales_bus {
	APB1, APB2, AHP
};

enum class rcc_clock_src {
	HSE, HSI, PLL
};

class rcc {
private:
	rcc_clock_src clock_src;
public:
	rcc(rcc_clock_src src_clock);
	void enable_peripheral_clock(periphrales_bus copy_bus, uint32_t copy_peripheral);
};

#endif /* CUSTOM_DRIVER_MCAL_RCC_RCC_H_ */
