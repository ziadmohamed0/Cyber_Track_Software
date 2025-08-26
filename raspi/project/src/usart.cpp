/**
 * @file usart.cpp
 * @author Ziad Fathy
 * @brief uart device driver source file.
 * @version 0.1
 * @date 2025-08-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "../inc/usart.hpp"

/**
 * @brief Construct a new UART::UART object
 * 
 * @param dev 
 * @param buad 
 */
UART::UART(const std::string &dev, speed_t buad) : 
            device(dev),
            buadRate(buad),
            fd(-1){}

/**
 * @brief 
 * 
 */
void UART::openPort() {
    this->fd = open(this->device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if(this->fd < 0 )
        std::runtime_error("Cannot open UART device");
    termios tty{};
    if(tcgetattr(this->fd, &tty) != 0)
        throw std::runtime_error("tcgetattr faild");

    cfsetospeed(&tty,this->buadRate);
    cfsetispeed(&tty, this->buadRate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                        // read doesn't block
    tty.c_cc[VTIME] = 5;                        // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);            // ignore modem controls
    tty.c_cflag &= ~(PARENB | PARODD);          // no parity
    tty.c_cflag &= ~CSTOPB;                     // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                    // no hardware flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        throw std::runtime_error("tcsetattr failed");
}

/**
 * @brief 
 * 
 * @param data 
 */
void UART::writeData(std::string data) {
    ssize_t n = ::write(this->fd, data.c_str(), data.size());
    if(n < 0) {
        throw std::runtime_error("UART write failed");
    }
}

/**
 * @brief 
 * 
 * @param maxLen 
 * @return std::string 
 */
std::string UART::readData(size_t maxLen) {
    char buf[maxLen];
    int n = ::read(this->fd, buf, maxLen);
    if(n < 0) 
        return "";
    return std::string(buf, n);
}

/**
 * @brief Destroy the UART::UART object
 * 
 */
UART::~UART(){
    if(this->fd >= 0 )
        close(fd);
}