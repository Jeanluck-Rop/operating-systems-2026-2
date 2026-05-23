#include "monitor.h"
#include "net_monitor.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

/*
  Punto de entrada del programa
  Uso:
  ./sysmon       //monitor de memoria y procesos
  ./sysmon --net //monitor de red
*/
int
main(int argc,
     char *argv[])
{
  ///*
  //Revisamos si se paso la bandera --net
  int mode_net = 0;
  for (int i = 1; i < argc; i++)
    if (strcmp(argv[i], "--net") == 0) mode_net = 1;
  
  if (mode_net) {
    NetSnapshot net_snapshot;
    // Inicializamos toda la estructura a cero antes de usarla, sin esto los campos
    //tendrian basura de memoria valores aleatorios que podrian hacer fallar los calculos 
    memset(&net_snapshot, 0, sizeof(net_snapshot));
    
    //Paso 1: leemos estadisticas de interfaces desde /proc/net/dev
    collect_net_ifaces(&net_snapshot);
    //Paso 2: leemos conexiones TCP/UDP y cruzamos con PIDs
    collect_net_conns(&net_snapshot);

    if (is_root()) {
      printf("[root] Leyendo nf_conntrack para datos de trafico...\n");
      collect_conntrack(&net_snapshot);
    } else {
      printf("[info] Corriendo sin root — "
	     "datos de trafico por conexion no disponibles.\n"
	     "       Para verlos: sudo ./build/src/sysmon --net\n\n");
    }
    
    //Paso 3: imprimimos todo lo reoclectado
    print_net_snapshot(&net_snapshot);
  }
  else {
    SystemSnapshot system_snapshot;
    //Inicializamos toda la estructura a cero antes de usarla, sin esto los campos
    ///tendrian basura de memoria valores aleatorios que podrian hacer fallar los calculos
    memset(&system_snapshot, 0, sizeof(system_snapshot));
    
    //Paso 1: leemos metricas de memoria desde /proc/meminfo
  collect_memory(&system_snapshot);
  //Paso 2: iteramos /proc y llenar el arreglo de procesos
  collect_processes(&system_snapshot);
  //Paso 3: imprimimos todo lo recolectado
  print_snapshot(&system_snapshot);
  } //*/
  //ui_run();
  return 0;
}
