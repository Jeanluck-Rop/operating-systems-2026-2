cmake_minimum_required(VERSION 3.13)

# Raspberry Pi Pico SDK import
include(pico_sdk_import.cmake)

# Proyect definition
project(practica3 C CXX ASM)

# SDK initialization
pico_sdk_init()

# Add the executable with ALL the pieces
add_executable(practica3
    main.c       # The entry point (main) and setup
    kernel_drivers.c    # Direct memory access (SIO)
    kernel_service.c    # Dispatcher in C (Switch/Case)
    svc_handler.s       # The assembly wrapper for the interrupt
    pendsv.s            # The assembly code for the context switch

    user_app.c          # Student code (LED/Button logic)
    syscalls.s          # Interface library (svc #0)
)

# Link libraries
# We use pico_stdlib for basic functions, even though we are making an OS
target_link_libraries(practica3
    pico_stdlib
    hardware_exception
)

# Generate artifacts (.uf2 to drag to the board)
pico_add_extra_outputs(practica3)
