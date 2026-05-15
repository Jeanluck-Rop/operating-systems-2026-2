# System Monitor

Herramienta de línea de comandos escrita en **C** que toma una fotografía del estado
de la memoria y los procesos activos en el sistema operativo Linux, y la imprime en
la terminal de forma ordenada.

## ¿Qué hace?

Al ejecutarse, el programa hace tres cosas en orden:

1. **Lee la memoria del sistema:** consulta cuánta RAM hay en total, cuánta está
   siendo usada por aplicaciones, cuánta está libre, y cuánto espacio de intercambio
   (swap) hay disponible.

2. **Escanea los procesos activos:** revisa cada proceso que está corriendo en el
   sistema y anota su nombre, cuánta memoria está ocupando en este momento, su estado
   (ejecutándose, dormido, esperando), y qué proceso lo inició.

3. **Imprime el resultado:** muestra primero un resumen de memoria y luego una tabla
   de procesos ordenada de mayor a menor consumo, terminando con el proceso que más y
   el que menos memoria usa.


## ¿Cómo funciona?

El programa lee directamente el sistema de archivos virtual `/proc`, que es una interfaz que el kernel de Linux
expone para que cualquier programa pueda consultar el estado interno del sistema operativo sin necesidad de
privilegios especiales.

| Fuente | Qué contiene |
|---|---|
| `/proc/meminfo` | Métricas globales de RAM y swap |
| `/proc/[pid]/comm` | Nombre corto del proceso |
| `/proc/[pid]/cmdline` | Comando completo con argumentos |
| `/proc/[pid]/status` | Estado, PID padre, RSS y tamaño virtual |

**RSS (Resident Set Size)** es la cantidad de RAM física que el proceso tiene cargada en este momento. Es la
métrica más útil para saber quién está consumiendo memoria real, a diferencia del tamaño virtual que incluye
memoria reservada pero no necesariamente cargada.


## Requisitos

- Linux (el programa depende de `/proc`, exclusivo de Linux)
- GCC
- CMake 3.10 o superior
- Docker (opcional, para correrlo sin instalar nada)


## Compilar y ejecutar localmente

```bash
git clone <url-del-repositorio>
```
```
cd system-monitor
```
```
cmake -B build && cmake --build build && ./build/src/sysmon
```


## Ejecutar con Docker

Docker permite correr el programa en cualquier máquina Linux (Fedora, Ubuntu, Debian, Arch) sin necesidad de
instalar GCC ni CMake.

**Construir la imagen:**

```bash
docker build -t sysmon .
```

**Ejecutar:**

```bash
docker run --rm --pid=host sysmon
```

La opción `--pid=host` es necesaria para que el contenedor pueda leer `/proc` de la máquina real y ver los
procesos del sistema huésped. Sin ella, el contenedor solo vería sus propios procesos internos.

## Ejemplo de salida

```
============== Memoria ==============
  Total:     15959212 KB  (15585 MB)
  Usada:      4566776 KB  (4459 MB)
  Libre:      1879340 KB  (1835 MB)
  Buffers:        328 KB  (0 MB)
  Cache:      9512768 KB  (9289 MB)
  Swap:        374000 KB usados / 8388604 KB total

======================= Procesos con memoria (169) =======================
PID     PPID    Nombre            RSS KB      VmSize KB   Estado    Padre            Comando
──────  ──────  ────────────────  ──────────  ──────────  ────────  ───────────────  ──────────────────────────────
2772    2478    gnome-software    742068      2221940     sleeping  gnome-session-b  /usr/bin/gnome-software --gapplication-s
3752    3416    Isolated Web Co   551472      3485468     sleeping  firefox          /usr/lib64/firefox/firefox -contentproc 
17215   3416    Isolated Web Co   533480      3518712     sleeping  firefox          /usr/lib64/firefox/firefox -contentproc 
3416    2543    firefox           512064      4353204     sleeping  gnome-shell      /usr/lib64/firefox/firefox

...

  Mayor consumidor : gnome-software  (PID 2772)  742068 KB
  Menor consumidor : p11-kit-server  (PID 6033)  1188 KB
```
