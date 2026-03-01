cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(bare_metal_blink C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializar el entorno del SDK
pico_sdk_init ()

# Definir el ejecutable con nuestro codigo fuente
add_executable(blink
  main.c
)

# --- PUNTO CRITICO ---
# Enlazamos ’ pico_stdlib ’ SOLO para que el Linker encuentre funciones base
# del sistema ( _exit , bootrom ) . NO violamos la regla de Bare Metal
# porque NO incluiremos < pico / stdlib .h > en nuestro codigo C .
target_link_libraries(blink
  pico_stdlib
)

# Generar archivos de salida (. uf2 , . hex , . map )
 pico_add_extra_outputs(blink)
