# Proyecto Final

## Monitor de Sistema en C: Memoria y Red

### Sistemas Operativos [2026-2]
#### Rojo Peña Manuel Ianluck
##### Fecha de entrega: 27 de mayo de 2026

---

Implementación de una herramienta de monitoreo de sistema escrita en lenguaje C que reporta el estado de la
memoria, los procesos activos y las conexiones de red del sistema operativo Linux, leyendo directamente desde el 
sistema de archivos virtual `/proc`.

---

## Organización del repositorio

```
project/
├── README.md
├── reporte/
│   ├── cover.tex           <- portada del reporte en LaTeX
│   ├── project.tex         <- reporte completo con todas las implementaciones
│   └── project.pdf         <- PDF compilado listo para entregar
└── system-monitor/
    ├── README.md           <- instrucciones de compilación y uso
    ├── CMakeLists.txt      <- configuración raíz de CMake
    ├── include/
    │   ├── monitor.h       <- estructuras y funciones del monitor de memoria
    │   ├── net_monitor.h   <- estructuras y funciones del monitor de red
    │   └── ui.h            <- estructuras y funciones de la interfaz ncurses
    └── src/
        ├── CMakeLists.txt  <- fuentes y bibliotecas
        ├── main.c          <- punto de entrada
        ├── monitor.c       <- lectura de /proc para memoria y procesos
        ├── net_monitor.c   <- lectura de /proc/net para interfaces y conexiones
        └── ui.c            <- interfaz de terminal con ncurses
```

El directorio `reporte/` contiene el reporte técnico en LaTeX que documenta cada etapa del desarrollo. 
El directorio `system-monitor/` contiene la implementación completa en C con sus instrucciones de uso.
