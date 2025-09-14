/*
 * rcc.cpp
 *
 *  Created on: Sep 3, 2025
 *      Author: ziad
 */
#include "rcc.h"

rcc::rcc(rcc_clock_src src_clock) : clock_src(src_clock) {
	switch (this->clock_src) {
		case rcc_clock_src::HSE:
			bit_math::set_bit(RCC->CR, 16);
			break;
		case rcc_clock_src::HSI:
			bit_math::set_bit(RCC->CR, 0);
			break;
		case rcc_clock_src::PLL:
			bit_math::set_bit(RCC->CR, 24);
			break;
		default:
			/* error state */
			break;
	}
}

void rcc::enable_peripheral_clock(periphrales_bus copy_bus, uint32_t copy_peripheral) {
	switch(copy_bus) {
	case periphrales_bus::APB1:
		bit_math::set_bit(RCC->APB1ENR, copy_peripheral);
		break;
	case periphrales_bus::APB2:
		bit_math::set_bit(RCC->APB2ENR, copy_peripheral);
		break;
	case periphrales_bus::AHP:
		bit_math::set_bit(RCC->AHBENR, copy_peripheral);
		break;
	default:
		/* error state */
		break;
	}
}
