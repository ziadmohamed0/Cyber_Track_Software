/*
 * gpio.cpp
 *
 *  Created on: Sep 3, 2025
 *      Author: ziad
 */

#include "gpio.h"

gpio::gpio(GPIO_PORT port,uint32_t pin, GPIO_STATUS dir, GPIO_CONFIG cfg) :
				rcc(rcc_clock_src::HSE),
				direction(dir),
				port_index(port),
				pin_index(pin),
				pin_cfg(cfg){
	switch (this->port_index) {
		case GPIO_PORT::A:
			this->enable_peripheral_clock(periphrales_bus::APB2, clock_gpio_a);
			break;
		case GPIO_PORT::B:
			this->enable_peripheral_clock(periphrales_bus::APB2, clock_gpio_b);
			break;
		case GPIO_PORT::C:
			this->enable_peripheral_clock(periphrales_bus::APB2, clock_gpio_c);
			break;
		default:
			/* error state */
			break;
	}
	this->init();
}

void gpio::set() {
	switch (this->port_index) {
		case GPIO_PORT::A:
			GPIOA->BSRR = (1U << this->pin_index);
			break;
		case GPIO_PORT::B:
			GPIOB->BSRR = (1U << this->pin_index);
			break;
		case GPIO_PORT::C:
			GPIOC->BSRR = (1U << this->pin_index);
			break;
		default:
			/* error state */
			break;
	}
}

void gpio::clear(){
	switch (this->port_index) {
		case GPIO_PORT::A:
			GPIOA->BSRR = (1U << (this->pin_index + 16));
			break;
		case GPIO_PORT::B:
			GPIOB->BSRR = (1U << (this->pin_index + 16));
			break;
		case GPIO_PORT::C:
			GPIOC->BSRR = (1U << (this->pin_index + 16));
			break;
		default:
			/* error state */
			break;
	}
}

bool gpio::get(){
	uint32_t ret_val{0};
	switch (this->port_index) {
		case GPIO_PORT::A:
			ret_val = (GPIOA->IDR & (1 << pin_index)) != 0;
			break;
		case GPIO_PORT::B:
			ret_val = (GPIOB->IDR & (1 << pin_index)) != 0;
			break;
		case GPIO_PORT::C:
			ret_val = (GPIOC->IDR & (1 << pin_index)) != 0;
			break;
		default:
			/* error state */
			break;
	}
	return ret_val;
}

void gpio::init() {
	volatile uint32_t *CR;
    uint8_t shift;
	switch (this->port_index) {
		case GPIO_PORT::A:
			if(this->pin_index < 8){
	            CR = &GPIOA->CRL;
	            shift = this->pin_index * 4;
			}
			else {
	            CR = &GPIOA->CRH;
	            shift = (this->pin_index - 8) * 4;
			}
			break;
		case GPIO_PORT::B:
			if(this->pin_index < 8){
	            CR = &GPIOB->CRL;
	            shift = this->pin_index * 4;
			}
			else {
	            CR = &GPIOB->CRH;
	            shift = (this->pin_index - 8) * 4;
			}
			break;
		case GPIO_PORT::C:
			if(this->pin_index < 8){
	            CR = &GPIOC->CRL;
	            shift = this->pin_index * 4;
			}
			else {
	            CR = &GPIOC->CRH;
	            shift = (this->pin_index - 8) * 4;
			}
			break;
		default:
			break;
	}
	*CR &= ~(0xF << shift);
    uint32_t val = (static_cast<uint32_t>(direction) |
                    (static_cast<uint32_t>(pin_cfg) << 2));
    *CR |= (val << shift);

    if (this->pin_cfg == GPIO_CONFIG::PULL_UP_DOWN && direction == GPIO_STATUS::INPUT) {
        switch (this->port_index) {
            case GPIO_PORT::A:
                bit_math::set_bit(GPIOA->ODR, this->pin_index); // pull-up
                break;
            case GPIO_PORT::B:
                bit_math::set_bit(GPIOB->ODR, this->pin_index);
                break;
            case GPIO_PORT::C:
                bit_math::set_bit(GPIOC->ODR, this->pin_index);
                break;
        }
    }
}
