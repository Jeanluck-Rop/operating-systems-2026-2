cmake_minimum_required(VERSION 3.13)

# Raspberry Pi Pico SDK import
include(pico_sdk_import.cmake)

# Proyect definition
project(practica2_syscalls C CXX ASM)

# SDK initialization
pico_sdk_init()

# Add the executable with ALL the pieces
add_executable(practica2_syscalls
    main.c       # The entry point (main) and setup
    kernel_drivers.c    # Direct memory access (SIO)
    kernel_service.c    # Dispatcher in C (Switch/Case)
    svc_handler.S       # The assembly wrapper for the interrupt
    
    user_app.c          # Student code (LED/Button logic)
    syscalls.S          # Interface library (svc #0)
)

# Link libraries
# We use pico_stdlib for basic functions, even though we are making an OS
target_link_libraries(practica2_syscalls 
    pico_stdlib
    hardware_exception
)

# Generate artifacts (.uf2 to drag to the board)
pico_add_extra_outputs(practica2_syscalls)
