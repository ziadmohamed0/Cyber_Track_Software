cmake_minimum_required(VERSION 3.10)

project(device_driver)

set( SRC
    main.cpp
    src/usart.cpp
)

include_directories(inc)

add_executable(${PROJECT_NAME} ${SRC})