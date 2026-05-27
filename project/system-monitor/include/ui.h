#ifndef UI_H
#define UI_H

#include "monitor.h"
#include "net_monitor.h"

/* Modos de visualizacion del monitor */
typedef enum {
  MODE_MEMORY = 0, //Mostramos memoria y procesos
  MODE_NET = 1   //Mostramos interfaces y conexiones de red
} DisplayMode;

/* Estado de la UI, se mantiene entre frames */
typedef struct {
  DisplayMode mode;  //modo activo actualmente
  int scroll_offset; //primera fila visible en la tabla
  int running;       //0 = salir del loop principal
} UIState;

/* Punto de entrada, toma control de la terminal hasta que el usuario salga */
void ui_run();

#endif /* UI_H */
