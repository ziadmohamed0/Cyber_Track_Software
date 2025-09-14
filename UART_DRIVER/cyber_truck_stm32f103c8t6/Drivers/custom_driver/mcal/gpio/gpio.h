/*
 * gpio.h
 *
 *  Created on: Sep 3, 2025
 *      Author: ziad
 */

#ifndef CUSTOM_DRIVER_MCAL_GPIO_GPIO_H_
#define CUSTOM_DRIVER_MCAL_GPIO_GPIO_H_

#include "../../lib/common.h"
#include  "../rcc/rcc.h"

enum class GPIO_PORT : uint8_t {
	A, B, C
};

enum class GPIO_STATUS {
    INPUT = 0b00,
    OUTPUT_10MHz = 0b01,
    OUTPUT_2MHz = 0b10,
    OUTPUT_50MHz = 0b11
};

enum class GPIO_CONFIG {
    // Input
    ANALOG = 0b00,
    FLOATING = 0b01,
    PULL_UP_DOWN = 0b10,

    // Output
    GP_PUSH_PULL = 0b00,
    GP_OPEN_DRAIN = 0b01,
    AF_PUSH_PULL = 0b10,
    AF_OPEN_DRAIN = 0b11
};

class gpio  : public rcc {
public:
	gpio(GPIO_PORT port,uint32_t pin, GPIO_STATUS dir, GPIO_CONFIG cfg);
	void set();
	void clear();
	bool get();
private:
	GPIO_STATUS direction;
	GPIO_PORT port_index;
	uint32_t pin_index;
	GPIO_CONFIG pin_cfg;
	void init();
};

#endif /* CUSTOM_DRIVER_MCAL_GPIO_GPIO_H_ */
