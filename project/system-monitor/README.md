# System Monitor

Herramienta de monitoreo de sistema escrita en **C** con interfaz de terminal
interactiva construida con **ncurses**. Se muestra en tiempo real el estado de la
memoria, los procesos activos y las conexiones de red del sistema operativo
Linux, leyendo directamente desde el sistema de archivos virtual `/proc`.

---

## Requisitos

- Linux (el programa depende de `/proc`, exclusivo de Linux)
- GCC
- CMake 3.10 o superior
- `libncurses-dev`

```bash
# Fedora
sudo dnf install gcc cmake ncurses-devel
```
```bash
# Debian / Ubuntu
sudo apt install gcc cmake libncurses-dev
```

---

## Compilar y ejecutar

```bash
cd system-monitor
```
```bash
cmake -B build && cmake --build build
```

Lanzar el monitor (modo memoria por defecto):

```bash
./build/src/sysmon
```

Una vez dentro, la tecla **N** cambia al modo red, el modo red requiere
privilegios de root, el programa lo solicita automáticamente con `sudo` la
primera vez que se presiona N, una vez ingresada la clave sudo, se debe volver a presionar la tecla **N** cuando el sistema haya cargado de nuevo la terminal en modo memoria. La tecla **q** cierra el monitor y restaura
la terminal a su estado original.

Para habilitar los contadores de tráfico por conexión individual (una sola vez):

```bash
sudo sysctl net.netfilter.nf_conntrack_acct=1
```

---

## Demostración

▶ [Ver demostración, vista general del monitor](https://drive.google.com/file/d/1bQJ6OxZkE0bbDfdezbS2aBh6ArU4X4J3/view?usp=sharing)

---

## Modo memoria

▶ [Video 1, modo memoria: scroll y procesos](https://drive.google.com/file/d/1eH8UNXC7DaFQEdnOV6NUv7LGqMjoUVnq/view?usp=sharing)
▶ [Video 2, modo memoria: video 4K y consumo de RAM](https://drive.google.com/file/d/1BjeoeoHrL0ei-DMIeUyzbS0HTLUe4jg2/view?usp=sharing)

El header muestra **System Monitor [Memory]** a la izquierda y la fecha y hora
actuales a la derecha, actualizándose cada segundo, el contenido es una tabla
desplazable de todos los procesos del sistema ordenados de mayor a menor consumo
de RAM. El footer muestra la barra de uso de RAM, el swap y el número de procesos
detectados.

### Columnas

| Columna | Muestra |
|---|---|
| PID | Identificador único del proceso asignado por el kernel |
| PPID | PID del proceso padre que lo creó |
| Nombre | Nombre corto del ejecutable (máximo 15 caracteres) |
| RSS KB | Memoria RAM física ocupada en este momento (ver abajo) |
| VmSize KB | Tamaño total del espacio de direcciones virtual reservado |
| Estado | Estado actual del proceso |
| Padre | Nombre del proceso padre |
| Comando | Ruta completa del ejecutable con sus argumentos |

### RSS KB y VmSize KB

**RSS (Resident Set Size)** es la cantidad de RAM que el proceso tiene
físicamente cargada en este momento, es la métrica real de consumo de memoria.

**VmSize** es el espacio de direcciones virtuales total que el proceso tiene
reservado, incluye código, bibliotecas, heap, stack y regiones mapeadas que
pueden estar en disco, en swap o simplemente reservadas pero nunca usadas. Por
eso VmSize siempre es mayor que RSS y a veces mucho mayor.

El caso más llamativo es Chrome, en la segunda captura se ve un proceso de Chrome
con VmSize de **1,518,717,176 KB** (aproximadamente 1.4 TB virtual) mientras su
RSS es de solo 209,912 KB (unos 200 MB reales). Esto ocurre porque Chrome
reserva un espacio de direcciones enorme al arrancar para gestionar la memoria de
las pestañas, pero la mayor parte de ese espacio nunca se llena con datos reales.
El sistema operativo no asigna RAM física hasta que realmente se necesita
reservar espacio virtual es gratuito en términos de memoria real.

### ¿Por qué casi todos aparecen como sleeping?

La columna Estado refleja lo que el proceso está haciendo en el instante exacto
en que el kernel lo consulta. Los valores posibles son:

- **sleeping**: el proceso está esperando un evento: una tecla, un dato de red,
  una respuesta del disco. Es el estado normal de cualquier aplicación que no
  está haciendo nada en este preciso momento, la mayoría de los procesos pasan
  el 99% de su vida en este estado.
- **running**: el proceso está ejecutando instrucciones en la CPU ahora mismo,
  o está en la cola listo para ejecutarse, este estado es tan breve que rara
  vez se ve en el monitor salvo que el proceso esté bajo carga intensa.
- **waiting**: esperando una operación de I/O (disco, red) que no puede
  interrumpirse.
- **zombie**: el proceso terminó pero su padre aún no ha leído su código de
  salida.

Gnome Shell, Firefox y la mayoría de aplicaciones gráficas son programas
orientados a eventos, hacen trabajo solo cuando el usuario interactúa con ellos
o llega datos de red. Entre eventos simplemente duermen, por eso aparecen como
sleeping aunque estén "abiertos". Cuando el usuario hace clic o llega un
mensaje, el proceso pasa brevemente a running para procesar el evento y vuelve
a sleeping de inmediato, tan rápido que en muchos casos el monitor no alcanza
a capturar ese estado.

### ¿Por qué cambiaban los valores de RSS y VmSize?

- El kernel puede mover páginas de memoria entre RAM y swap según la presión
  del sistema, lo que hace que RSS suba o baje incluso sin que el proceso haga
  nada explícito.
- Al cargar Chrome y el video 4K, el sistema necesitó más RAM y el kernel
  reajustó la distribución de páginas entre todos los procesos activos.
- Firefox tiene procesos hijos (Isolated Web Co, Privileged Cont) que comparten
  bibliotecas con el proceso principal, cuando el kernel mueve o consolida esas
  páginas compartidas, los números de RSS de varios procesos cambian a la vez.

El proceso que pasó a **running** en la segunda captura fue `xdg-desktop-por`
(PID 3473), que es el portal de escritorio XDG, el componente que intermedia
entre aplicaciones y el sistema para cosas como seleccionar archivos, acceder
al portapapeles y gestionar notificaciones. Al abrir Chrome, este portal se
activa para registrar la nueva aplicación en el entorno de escritorio.

### ¿Por qué el video 4K subió la RAM del 35% al 40%?

Un video 4K a 60fps requiere decodificar frames enormes, cada frame sin
comprimir de un video 4K ocupa aproximadamente 32 MB (3840x2160 píxeles x 4
bytes por píxel), el decodificador mantiene varios frames en memoria
simultáneamente para poder hacer predicción de movimiento. Además, Chrome
separa la decodificación en procesos distintos (`--type=renderer`, `--type=gpu`)
que aparecen como entradas independientes en la tabla. Todo eso se suma en RAM
real (RSS), y por eso la barra del footer sube gradualmente mientras el video
se reproduce.

### Colores de las filas

- **Rojo**: el proceso usa más de 500 MB de RAM: gnome-software (814 MB), 
los Isolated Web Co de Firefox (tabs pesadas), y Chrome con el video 4K.
- **Amarillo/naranja**: entre 100 MB y 500 MB, procesos medianos como gnome-shell,
  firefox principal, xdg-desktop-portal.
- **Blanco**: menos de 100 MB, la mayoría de servicios del sistema.

### Footer del modo memoria

```
RAM 5603/15585 MB [#######.............] 35%  |  Swap 2253/8191 MB  |  procs: 156
Arriba/Abajo: scroll   N: red   q: salir
```

La barra muestra la RAM usada sobre el total, los bloques `#` se colorean en
verde (< 60%), amarillo (60–85%) o rojo (> 85%) según el porcentaje. El swap
indica cuánto espacio de intercambio en disco está en uso, cuando la RAM se
llena el kernel mueve páginas al swap para liberar espacio, lo que hace el
sistema más lento. Se recomienda usar las teclas de flecha para hacer scroll
en lugar del ratón, la respuesta del teclado es inmediata mientras que el
scroll del ratón puede enviar múltiples eventos seguidos que la terminal
procesa con retraso.

---

## Modo red

▶ [Video 3, modo red](https://drive.google.com/file/d/1KlZrkW6NJNn2kV4peSRV-cHlzo11n_Vt/view?usp=sharing)
▶ [Video 4, modo red](https://drive.google.com/file/d/1chG-VN2fYDpQYX__nIThuY6ReuIU5FMp/view?usp=sharing)

Al presionar **N** desde el modo memoria, el programa solicita autenticación
sudo si no se está corriendo como root y se relanza con privilegios. El header
muestra **System Monitor [Net] [root]** confirmando que se tienen los permisos
necesarios.

### Sección de interfaces físicas

Solo se muestran las interfaces con hardware real, en los ejemplos aparece
únicamente `wlp1s0`, que es la tarjeta WiFi física del sistema. El nombre sigue
el estándar de nomenclatura predictiva de Linux: `wl` = wireless, `p1` = bus
PCI 1, `s0` = slot 0.

| Columna | Muestra |
|---|---|
| Interfaz | Nombre del dispositivo de red |
| RX bytes | Total de bytes recibidos desde que arrancó el sistema |
| TX bytes | Total de bytes transmitidos desde que arrancó el sistema |
| RX pkts | Total de paquetes recibidos |
| TX pkts | Total de paquetes transmitidos |

Estos valores **no son velocidad actual**, son contadores acumulados desde el
boot, por eso crecen constantemente entre capturas. Para calcular velocidad en
KB/s se necesitarían dos lecturas separadas por un intervalo de tiempo, lo cual
es posible en una implementación futura con hilos.

### Conexiones activas

La tabla muestra todas las conexiones TCP y UDP del sistema que pudieron
asociarse a un proceso, las conexiones en verde son ESTABLISHED (activas), las
amarillas son SYN_SENT (intentando conectarse).

**Por qué de que haya pocas conexiones en pantalla aunque el footer diga 27 o 46:**
las primeras filas siempre son las conexiones en estado LISTEN, servicios del
sistema esperando conexiones entrantes que no tienen dirección remota, no son
conexiones activas hacia internet sino puertos abiertos localmente:

- **systemd-resolve** en puertos 53 y 5355, el resolvedor de DNS del sistema,
  escucha peticiones de otros programas que quieran convertir nombres de dominio
  a IPs.
- **passimd** en puerto 27500, servicio de gestión de contraseñas del sistema.
- **cupsd** en puerto 631, el servidor de impresión CUPS, siempre escuchando
  aunque no haya impresoras activas.
- **sshd** en puerto 22, el servidor SSH, permite conexiones remotas al equipo.

Las conexiones ESTABLISHED hacia internet aparecen después de estas, coloreadas
en verde.

### ¿Qué significan los hostnames en Remoto/Host?

El monitor intenta resolver cada IP remota a su nombre de dominio mediante DNS
inverso, esto hace los resultados mucho más legibles, ejemplos:

- `93.243.107.34.bc.googleusercontent.com`: servidor de Google Cloud, usado
  por Firefox para cargar contenido desde servicios de Google.
- `tzqroa-ao-in-f14.1e100.net` y `pnqroa-ac-in-f2.1e100.net`: servidores de
  Google, el dominio `1e100.net` es propiedad de Google (1e100 = 10^100 =
  googol), son servidores de la infraestructura de Google usados por Chrome
  para sincronización, actualizaciones y telemetría.
- `vps40588.dreamhostps.com`, servidor privado virtual en DreamHost, un
  proveedor de hosting. Chrome se conectó a este durante la navegación, pertenece a 
  la conexión con una página de Tetris.
- `_gateway`: el router de la red local (192.168.1.1), aparece así porque
  NetworkManager lo registra en el DNS local con ese nombre.

### ¿Por qué las conexiones del video 4K desaparecieron en el video?

Cuando YouTube reproduce un video en 4K usa el protocolo QUIC sobre UDP (por
eso aparece como UDP ESTABLISHED hacia servidores de Google). QUIC es más
eficiente que TCP para streaming porque no necesita retransmitir paquetes
perdidos. Sin embargo, las conexiones QUIC/UDP tienen una vida más corta que
las TCP, YouTube abre y cierra conexiones UDP constantemente mientras hace
streaming adaptativo (ajusta la calidad según el ancho de banda disponible).
Al cambiar de modo o al pasar unos segundos, las conexiones que se veían antes
ya se cerraron y se abrieron otras nuevas hacia diferentes servidores del CDN
de Google. Por eso en la segunda captura de red ya no aparecen los mismos
hostnames aunque el video siga reproduciéndose.


### Footer del modo red

```
Total RX: 5760.3 MB   TX: 350.0 MB   Interfaces: 4   Conexiones: 28
Arriba/Abajo: scroll   N: memoria   q: salir
```

El total RX/TX suma únicamente las interfaces físicas, excluyendo loopback y
bridges virtuales de Docker, los valores crecen continuamente porque son
acumulados desde el arranque.

---

## Navegación

| Tecla | Acción |
|---|---|
| `↑` / `↓` | Scroll por la tabla de procesos o conexiones |
| `N` | Cambiar entre modo memoria y modo red |
| `q` | Salir y restaurar la terminal |

Se recomienda usar las teclas de flecha en lugar del scroll del ratón, el
teclado envía un evento por pulsación mientras que el ratón puede acumular
varios eventos seguidos que llegan todos a la vez, haciendo que la tabla
salte varias posiciones de golpe.
