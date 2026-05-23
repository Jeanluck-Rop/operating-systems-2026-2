# System Monitor

Herramienta de línea de comandos escrita en **C** que reporta el estado de la
memoria, los procesos activos y las conexiones de red del sistema operativo
Linux, leyendo directamente desde el sistema de archivos virtual `/proc`.

---

## ¿Qué hace?

El programa tiene dos modos de operación seleccionables al momento de ejecutarlo.

### Modo memoria (por defecto)

Al ejecutarse sin argumentos, el programa hace tres cosas en orden:

1. **Lee la memoria del sistema** — consulta cuánta RAM hay en total, cuánta está
   siendo usada por aplicaciones, cuánta está libre, y cuánto espacio de
   intercambio (swap) hay disponible.

2. **Escanea los procesos activos** — revisa cada proceso que está corriendo en
   el sistema y anota su nombre, cuánta memoria está ocupando en este momento,
   su estado (ejecutándose, dormido, esperando) y qué proceso lo inició.

3. **Imprime el resultado** — muestra primero un resumen de memoria y luego una
   tabla de procesos ordenada de mayor a menor consumo, terminando con el proceso
   que más y el que menos memoria usa.

### Modo red (`--net`)

Al ejecutarse con la bandera `--net`, el programa:

1. **Lee las interfaces de red** — obtiene el total de bytes y paquetes enviados
   y recibidos por cada interfaz desde el arranque del sistema, distinguiendo
   entre interfaces físicas (WiFi, Ethernet) y virtuales (loopback, Docker,
   bridges).

2. **Escanea las conexiones activas** — lee las conexiones TCP y UDP del sistema
   y las cruza con los procesos que las abrieron, mostrando qué aplicación está
   hablando con qué servidor.

3. **Enriquece con datos de tráfico** — si se ejecuta como root, consulta además
   el módulo `nf_conntrack` del kernel para obtener los bytes enviados y recibidos
   por cada conexión individual.

---

## ¿Cómo funciona?

El programa lee el sistema de archivos virtual `/proc`, una interfaz que el kernel
de Linux expone para que cualquier programa consulte el estado interno del sistema
operativo.

### Fuentes del modo memoria

| Fuente | Qué contiene |
|---|---|
| `/proc/meminfo` | Métricas globales de RAM y swap |
| `/proc/[pid]/comm` | Nombre corto del proceso |
| `/proc/[pid]/cmdline` | Comando completo con argumentos |
| `/proc/[pid]/status` | Estado, PID padre, RSS y tamaño virtual |

**RSS (Resident Set Size)** es la cantidad de RAM física que el proceso tiene
cargada en este momento. Es la métrica más útil para saber quién está consumiendo
memoria real, a diferencia del tamaño virtual que incluye memoria reservada pero
no necesariamente cargada en RAM.

### Fuentes del modo red

| Fuente | Qué contiene | Requiere root |
|---|---|---|
| `/proc/net/dev` | Bytes y paquetes por interfaz | No |
| `/proc/net/tcp` | Conexiones TCP activas en hex | No |
| `/proc/net/udp` | Conexiones UDP activas en hex | No |
| `/proc/[pid]/fd/` | File descriptors para cruzar con sockets | Parcial |
| `/proc/net/nf_conntrack` | Bytes por conexión individual | Sí |

---

## Requisitos

- Linux (el programa depende de `/proc`, exclusivo de Linux)
- GCC
- CMake 3.10 o superior
- `libncurses-dev` (para la interfaz de terminal)

Instalar dependencias:

```bash
# Fedora
sudo dnf install gcc cmake ncurses-devel

# Debian / Ubuntu
sudo apt install gcc cmake libncurses-dev
```

---

## Compilar y ejecutar

Asegurarnos de estar en `system-monitor`:
```bash
cd system-monitor
```

Una vez estams en el directorio correcto:
```bash
cmake -B build && cmake --build build
```

**Monitor de memoria:**

```bash
./build/src/sysmon
```

**Monitor de red** (sin root: conexiones sin datos de tráfico):

```bash
./build/src/sysmon --net
```

**Monitor de red** (con root: incluye bytes por conexión):

```bash
sudo ./build/src/sysmon --net
```

Para habilitar la contabilidad de tráfico por conexión (necesario una sola vez):

```bash
sudo sysctl net.netfilter.nf_conntrack_acct=1
```

---

## Ejemplos de salida

### Modo memoria

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

### Modo red

```
=== Interfaces físicas ===
Interfaz       RX bytes         TX bytes       RX pkts      TX pkts
────────────   ───────────────  ─────────────  ────────────  ────────────
wlp1s0         2057407916       75431369       1842257       296104

=== Interfaces virtuales ===
  lo            RX: 459013 bytes  TX: 459013 bytes
  docker0       RX: 4991262 bytes  TX: 231581132 bytes

  Total RX: 1.9 GB
  Total TX: 71.9 MB

=== Conexiones con proceso identificado (11) ===
PID    Proto  Local                    Remoto                         Estado         Proceso
─────  ─────  ───────────────────────  ─────────────────────────────  ─────────────  ────────────────
3688   TCP    192.168.100.33:35388     31.13.89.53:443                ESTABLISHED    firefox
3688   TCP    192.168.100.33:41468     140.82.112.26:443              ESTABLISHED    firefox
29209  TCP    192.168.100.33:43806     66.90.87.106:443               ESTABLISHED    chrome
  ^42.7 KB   v2.9 MB   pkts ^792/v1014
```
