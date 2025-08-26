/**
 * @file main.cpp
 * @author Ziad Fathy
 * @brief 
 * @version 0.1
 * @date 2025-08-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "inc/usart.hpp"

int main(void) {
    UART uart("/dev/serial0", B115200);
    uart.openPort();

    uart.writeData("Hello from Raspberry Pi 4\n");

    std::string response = uart.readData();
    std::cout << "Received: " << response << "\n";
    return 0;
}