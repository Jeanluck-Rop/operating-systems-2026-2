# UAF + CFI Demo

Un solo archivo fuente compilado de tres formas distintas.
El codigo es identico en los tres casos. Lo que cambia son
los flags del compilador y el comportamiento resultante.

---

## Archivo fuente

uaf.cpp

---

## Comando 1: UAF puro (sin protecciones)

    clang++ -O0 -std=c++17 -o uaf uaf.cpp
    ./uaf

Flags utilizados:
  -O0          sin optimizaciones, el compilador no modifica el codigo
  -std=c++17   version del estandar de C++

Comportamiento al ejecutar:
  El programa crea un objeto Animal en el heap y lo libera con delete.
  Un objeto Exploit es asignado en ese mismo bloque de memoria.
  Cuando el programa llama animal->speak() a traves del puntero
  colgante, la vtable en esa direccion apunta a Exploit::speak
  y el codigo del atacante se ejecuta sin ninguna advertencia ni error.

  Este es el ataque Use-After-Free en su forma mas basica.

---

## Comando 2: deteccion con reporte completo (AddressSanitizer)

    clang++ -O1 -std=c++17 -fsanitize=address -fno-omit-frame-pointer -g -o cfi uaf.cpp
    ./cfi

Flags utilizados:
  -O1                      optimizacion minima, requerida por ASan
  -fsanitize=address       activa AddressSanitizer
  -fno-omit-frame-pointer  conserva el frame pointer para stack traces
  -g                       incluye informacion de debug con numeros de linea

Comportamiento al ejecutar:
  AddressSanitizer instrumenta cada acceso a memoria en tiempo de
  ejecucion. Al intentar usar animal->speak() sobre memoria ya liberada,
  el programa aborta con un reporte detallado:

    ERROR: AddressSanitizer: heap-use-after-free
    READ of size 8 at 0x... thread T0
    #0 main uaf.cpp:XX
    freed by thread T0 here:
    #0 operator delete uaf.cpp:XX

  El reporte indica la linea exacta donde se libero la memoria,
  la linea donde se intento usar y el stack trace completo.

---

## Comando 3: bloqueo silencioso (Control-Flow Integrity)

    clang++ -O1 -std=c++17 -fsanitize=cfi -flto -fvisibility=hidden -g -o uaf_cfi uaf.cpp
    ./uaf_cfi

Flags utilizados:
  -fsanitize=cfi       activa Control-Flow Integrity
  -flto                Link Time Optimization, permite a CFI analizar
                       toda la jerarquia de clases del programa
  -fvisibility=hidden  oculta simbolos internos para que CFI pueda
                       determinar con exactitud que clases son legitimas

Comportamiento al ejecutar:
  Clang analiza en tiempo de compilacion todas las clases con metodos
  virtuales y construye el conjunto de destinos legitimos para cada
  llamada indirecta. Antes de cada animal->speak(), el binario verifica
  que la vtable en esa direccion pertenece al tipo Animal o sus subclases.
  Si no coincide, se invoca __cfi_check_fail() y el programa termina.

  El mensaje de salida es breve:
    Illegal instruction (core dumped)

  CFI no es una herramienta de diagnostico sino una proteccion de
  produccion. No genera reportes: simplemente detiene el ataque.
  Este comportamiento corresponde a lo descrito en la figura 9-34
  del libro, donde cada llamada indirecta incluye una verificacion
  de etiqueta antes de saltar al destino.

---

## Resumen

  ./uaf        el exploit se ejecuta sin ninguna interrupcion
  ./cfi        ASan detecta el UAF y aborta con reporte detallado
  ./uaf_cfi    CFI verifica la vtable y aborta antes del exploit

El orden de ejecucion refleja una progresion natural: primero se
observa el ataque funcionando, luego se introduce la deteccion en
tiempo de desarrollo, y finalmente la proteccion en produccion.
