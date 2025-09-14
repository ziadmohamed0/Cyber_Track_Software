/*
 * uart.cpp
 *
 *  Created on: Sep 5, 2023
 *      Author: MMohamedSaid
 */

#include "uart.h"

volatile char rxBuffer[RX_BUFFER_SIZE];
volatile char txBuffer[TX_BUFFER_SIZE];
volatile uint16_t rxHead = 0, rxTail = 0;
volatile uint16_t txHead = 0, txTail = 0;
volatile bool txBusy = false;

extern "C" void USART1_IRQHandler(void)
{
	if(USART1->SR & (1<<5))
	{
		char receivedData = USART1->DR & 0xFF;

		uint16_t nextHead = (rxHead + 1) % RX_BUFFER_SIZE;

		if(nextHead != rxTail)
		{
			rxBuffer[rxHead] = receivedData;
			rxHead = nextHead;
		}
	}

	if(USART1->SR & (1<<7))
	{
		if(txHead != txTail)
		{
			USART1->DR = txBuffer[txTail];
			txTail = (txTail + 1) % TX_BUFFER_SIZE;
		}

		else
		{
			USART1->CR1 &= ~(1<<7);
			txBusy = false;
		}
	}
}
