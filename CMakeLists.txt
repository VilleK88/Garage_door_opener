# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.17)

# Set board type because we are building for PicoW
set(PICO_BOARD pico_w)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C   standards
project(Garage_door_opener C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
        main.cpp
        main.h
        src/StepMotor.cpp
        src/StepMotor.h
        src/StateMachine.cpp
        src/StateMachine.h
        src/LimitSwitch.cpp
        src/LimitSwitch.h
        src/Eeprom.cpp
        src/Eeprom.h
        src/LedController.cpp
        src/LedController.h
        src/Wifi.cpp
        src/Wifi.h
        lwipopts.h
        config/wifi_config.h
        src/LoRaE5.cpp
        src/LoRaE5.h
        src/IPStack.cpp
        src/IPStack.h
        src/MqttService.cpp
        src/MqttService.h
        src/RotaryEncoder.cpp
        src/RotaryEncoder.h
        utils/events.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
        pico_stdlib
        hardware_pwm
        hardware_gpio
        hardware_i2c
        hardware_adc
        pico_util

        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
)

# Disable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)