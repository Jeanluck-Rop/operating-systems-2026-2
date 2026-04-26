cmake_minimum_required(VERSION 3.13)

# Pull in PICO SDK (Assuming it's available in the environment)
include(pico_sdk_import.cmake)

project(mpu_debug C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(mpu_debug
    main.c
)

# Pull in common dependencies
target_link_libraries(mpu_debug pico_stdlib hardware_exception)

# Enable USB output
pico_enable_stdio_usb(mpu_debug 1)
pico_enable_stdio_uart(mpu_debug 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(mpu_debug)
