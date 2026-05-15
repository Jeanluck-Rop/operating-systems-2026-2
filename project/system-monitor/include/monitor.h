#ifndef MONITOR_H
#define MONITOR_H

/* Bibliotecas estandar necesarias para los tipos y funciones
   que se usan en monitor.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

//Numero maximo de procesos que el snapshot puede almacenar
#define MAX_PROCS 512


/* Representa un unico proceso del sistema operativo,
   cada campo viene de una fuente distinta dentro de /proc/[pid]/ */
typedef struct
{
  int process_id;        //PID: identificador unico del proceso, fuente
  int parent_process_id; //PPID: PID del proceso que lo creo, fuente
  char name[256];        //Nombre corto del ejecutable, max 15 chars
  char cmdline[512];     //Comando completo con argumentos
  char parent_name[256]; //Nombre del proceso padre
  char state;            //Estado actual: R running, S sleeping, D waiting I/O, Z zombie, T stopped
  long vm_size_kb;       //Tamano total del espacio virtual en KB, incluye todo lo reservado aunque no esteen RAM
  long memory_kb;        //RSS en KB: paginas realmente en RAM ahora,  es la metrica de uso real de memoria
}
  ProcessInfo;


/* Fotografia completa del estado del sistema en un instante
   agrupa metricas de memoria global y la lista de procesos */
typedef struct
{
  long memory_total_kb;   //RAM total instalada
  long memory_used_kb;    //RAM en uso real = total - free - buffers - cached
  long memory_free_kb;    //RAM completamente libre
  long memory_buffers_kb; //Cache de metadatos de filesystem
  long memory_cached_kb;  //Cache de contenido de archivos
  long swap_total_kb;     //Espacio de intercambio total
  long swap_used_kb;      //Swap en uso = SwapTotal - SwapFree
  ProcessInfo processes[MAX_PROCS]; //Arreglo estatico de procesos encontrados
  int process_count;                //Cuantas entradas validas hay en processes[]
}
  SystemSnapshot;

/* */
void collect_memory(SystemSnapshot *system_snapshot);

/* */
void collect_processes(SystemSnapshot *system_snapshot);

/* */
void print_snapshot(const SystemSnapshot *system_snapshot);

#endif /* MONITOR_H */
