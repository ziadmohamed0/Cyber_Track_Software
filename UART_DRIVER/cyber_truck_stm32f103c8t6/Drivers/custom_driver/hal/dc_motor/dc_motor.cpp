/*
 * dc_motor.cpp
 *
 *  Created on: Sep 4, 2025
 *      Author: ziad
 */
#include "dc_motor.h"

DC_MOTOR* DC_MOTOR::pin_to_motor[16] = {nullptr};


DC_MOTOR::DC_MOTOR(motor_cfg_t* cfg) :
					m_cfg(cfg),
					pin1(cfg->port, cfg->pins[0], GPIO_STATUS::OUTPUT_50MHz, GPIO_CONFIG::GP_PUSH_PULL),
					pin2(cfg->port, cfg->pins[1], GPIO_STATUS::OUTPUT_50MHz, GPIO_CONFIG::GP_PUSH_PULL),
					pwm(cfg->pwm_tim, 72-1, 1000-1, nullptr),
					position(0){
	pin_to_motor[this->m_cfg->pins[0]] = this;
	pin_to_motor[this->m_cfg->pins[1]] = this;
    this->pwm.pwm_init(this->m_cfg->pwm_channel);
    encA = new external_interrupt(cfg->port, cfg->pins[0], exti_trigger::RISING, encoder_callback);
    encB = new external_interrupt(cfg->port, cfg->pins[1], exti_trigger::RISING, encoder_callback);
}

void DC_MOTOR::move(motor_direction copy_direction) {
	switch (copy_direction) {
		case motor_direction::forward:
			this->pin1.set();
			this->pin2.clear();
			break;
		case motor_direction::reverse:
			this->pin1.clear();
			this->pin2.set();
			break;
		case motor_direction::stop:
			this->pin1.clear();
			this->pin2.clear();
			break;
		default:
			this->pin1.clear();
			this->pin2.clear();
			break;
	}
}

void DC_MOTOR::set_speed(uint32_t duty_percent){
	this->pwm.pwm_set_duty(this->m_cfg->pwm_channel, duty_percent);
}

void DC_MOTOR::stop() {
	this->move(motor_direction::stop);
	this->pwm.pwm_set_duty(this->m_cfg->pwm_channel, 0);
}

int32_t DC_MOTOR::get_position() {
	return this->position;
}

void DC_MOTOR::reset_position() {
	this->position = 0;
}


// static callback from EXTI
void DC_MOTOR::encoder_callback(uint8_t pin) {
    if (pin_to_motor[pin]) {
        if (pin == pin_to_motor[pin]->m_cfg->pins[0])
            pin_to_motor[pin]->handle_encoderA();
        else if (pin == pin_to_motor[pin]->m_cfg->pins[1])
            pin_to_motor[pin]->handle_encoderB();
    }
}

void DC_MOTOR::handle_encoderA() {
    // Check state of B to know direction
    if (gpio(m_cfg->port, m_cfg->pins[0], GPIO_STATUS::INPUT, GPIO_CONFIG::FLOATING).get())
        position++;
    else
        position--;
}

void DC_MOTOR::handle_encoderB() {
    // Check state of A to know direction
    if (gpio(m_cfg->port, m_cfg->pins[1], GPIO_STATUS::INPUT, GPIO_CONFIG::FLOATING).get())
        position--;
    else
        position++;
}
