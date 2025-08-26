/**
 * @file usart.hpp
 * @author Ziad Fathy
 * @brief uart device driver header file. 
 * @version 0.1
 * @date 2025-08-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef UASRT_H_
#define UASRT_H_

/* --------- Includes --------- */
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <stdexcept>

/* --------- Class --------- */
class UART{
    public:
        UART(const std::string &dev, speed_t buad);
        void openPort();
        void writeData(std::string data);
        std::string readData(size_t maxLen = 256);
        ~UART();
    private:
        int fd;
        std::string device;
        speed_t buadRate; 
};

#endif