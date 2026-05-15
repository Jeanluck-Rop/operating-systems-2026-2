#include "monitor.h"
#include <string.h>

/* Punto de entrada del programa */
int
main()
{
  SystemSnapshot system_snapshot;
  /* Inicializamos toda la estructura a cero antes de usarla, sin esto los campos
     tendrian basura de memoria valores aleatorios que podrian hacer fallar los calculos */
  memset(&system_snapshot, 0, sizeof(system_snapshot));
  
  //Paso 1: leemos metricas de memoria desde /proc/meminfo
  collect_memory(&system_snapshot);
  //Paso 2: iteramos /proc y llenar el arreglo de procesos
  collect_processes(&system_snapshot);
  //Paso 3: imprimimos todo lo recolectado
  print_snapshot(&system_snapshot);

  return 0;
}
