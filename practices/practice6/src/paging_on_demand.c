#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "paging_on_demand.h"

#define MAX_FRAMES 128
#define MAX_PROCESSES 10
#define MAX_PAGES 64

typedef struct {
  int pid;
  int v_page;
  int is_page_table;
  int last_use;
} frame_info_t;

frame_info_t ram_frames[MAX_FRAMES];
int total_ram_frames = 0;
int demand_paging_timer = 0;

typedef struct {
  int pid;
  int page_table[MAX_PAGES]; // Maps v_page to frame index (-1 if not in RAM)
  int total_pages;
} process_demand_t;

process_demand_t demand_processes[MAX_PROCESSES];
int demand_process_count = 0;

/*
 * Inicializamos el subsistema de paginacion bajo demanda.
 * Limpiamos todos los marcos y reinicia el contador de procesos.
 */
void
init_demand_system(int frames)
{
  total_ram_frames = (frames > MAX_FRAMES) ? MAX_FRAMES : frames;
  for (int i = 0; i < MAX_FRAMES; i++) {
    ram_frames[i].pid = -1;
    ram_frames[i].is_page_table = 0;
    ram_frames[i].last_use = 0;
  }
  demand_process_count = 0;
  printf("SYSTEM: Demand Paging initialized with %d frames\n", total_ram_frames);
}

/*
 * Buscamos el primer marco fisico libre en RAM.
 * Recorremos secuencialmente los marcos hasta encontrar uno con pid == -1.
 */
int
find_free_frame()
{
  for (int i = 0; i < total_ram_frames; i++) {
    if(ram_frames[i].pid == -1)
      return i;
  }
  return -1;
}

/*
 * Desalojamos un marco usando la politica LRU Global.
 * Seleccionamos el marco con el menor valor de last_use (el menos
 * recientemente usado) como victima para desalojo.
 * Los marcos marcados como is_page_table == 1 estan protegidos (pinned)
 * y nunca se desalojan, ya que perder una tabla de paginas corromperia todo el proceso.
 * Tras seleccionar la victima:
 *   - Invalida la entrada correspondiente en la tabla de paginas del proceso
 *   - Limpia el marco para reutilizacion
 */
int
evict_frame()
{
  int victim = -1;
  int oldest_use = __INT_MAX__;
  
  //LRU Global: buscamos el frame con menor last_use, excluyendo tablas de paginas
  for (int i = 0; i < total_ram_frames; i++) {
    if (ram_frames[i].pid != -1 && ram_frames[i].is_page_table == 0) {
      if (ram_frames[i].last_use < oldest_use) {
	oldest_use = ram_frames[i].last_use;
	victim = i;
      }
    }
  }  
  if (victim != -1) {
    printf("EVICT: Frame %d (PID %d, Page %d) removed from RAM\n",
	   victim, ram_frames[victim].pid, ram_frames[victim].v_page);
    //Invalidamos la entrada en la tabla de paginas del proceso afectado
    for (int i = 0; i < demand_process_count; i++) {
      if (demand_processes[i].pid == ram_frames[victim].pid) {
	demand_processes[i].page_table[ram_frames[victim].v_page] = -1;
	break;
      }
    }
    //Limpiar el marco para que pueda ser reutilizado
    ram_frames[victim].pid = -1;
    ram_frames[victim].v_page = -1;
    ram_frames[victim].is_page_table = 0;
    ram_frames[victim].last_use = 0;
  }  
  return victim;
}


/*
 * Registra un nuevo proceso y reserva su tabla de paginas.
 * Reserva un marco exclusivo para la tabla de paginas del proceso y lo
 * marca como pinned (is_page_table = 1) para que nunca sea desalojado.
 * Las paginas de datos se cargan bajo demanda mediante access_page.
 */
void
load_process(int pid,
	     int pages)
{
  if (demand_process_count >= MAX_PROCESSES)
    return;
  //Reservamos un frame para la tabla de paginas del proceso
  int pt_frame = find_free_frame();
  if (pt_frame == -1)
    pt_frame = evict_frame();
  if (pt_frame != -1) {
    //Marcamos el frame como tabla de paginas (pinned, no se desaloja)
    ram_frames[pt_frame].pid = pid;
    ram_frames[pt_frame].v_page = -1;
    ram_frames[pt_frame].is_page_table = 1;
    ram_frames[pt_frame].last_use = ++demand_paging_timer;
    //Inicializamos el proceso
    demand_processes[demand_process_count].pid = pid;
    demand_processes[demand_process_count].total_pages = pages;
    for (int i = 0; i < MAX_PAGES; i++)
      demand_processes[demand_process_count].page_table[i] = -1;
    printf("LOAD: PID %d loaded. Page Table pinned at Frame %d\n", pid, pt_frame);
    demand_process_count++;
  }
}

/*
 * Simula el acceso a una pagina virtual de un proceso.
 * Flujo de ejecucion:
 *   1. Buscar el proceso en el arreglo de procesos registrados.
 *   2. Consultar la tabla de paginas para verificar si la pagina esta en RAM.
 *   3. Si esta (HIT): actualizar el timestamp last_use para LRU.
 *   4. Si no esta (PAGE FAULT):
 *      a. Buscar un marco libre o desalojar uno (LRU).
 *      b. Cargar la pagina desde disco al marco (PAGE IN).
 *      c. Actualizar la tabla de paginas con el nuevo mapeo.
 */
void
access_page(int pid,
	    int v_page)
{
  demand_paging_timer++;
  //Buscamos el proceso
  process_demand_t *proc = NULL;
  for (int i = 0; i < demand_process_count; i++) {
    if (demand_processes[i].pid == pid) {
      proc = &demand_processes[i];
      break;
    }
  }
  if (proc == NULL) {
    printf("ERROR: PID %d not found\n", pid);
    return;
  }
  //Verificamos si la pagina esta en RAM
  int frame = proc->page_table[v_page];
  if (frame != -1 && ram_frames[frame].pid == pid && ram_frames[frame].v_page == v_page) {
    //HIT: pagina en RAM, actualizamos last_use
    ram_frames[frame].last_use = demand_paging_timer;
    printf("ACCESS: PID %d, Page %d -> Hit at Frame %d\n", pid, v_page, frame);
  } else {
    //PAGE FAULT: pagina no esta en RAM
    printf("PAGE FAULT: PID %d, Page %d not in RAM\n", pid, v_page);
    //Buscamos frame libre o desalojar uno
    int new_frame = find_free_frame();
    if (new_frame == -1)
      new_frame = evict_frame();
    if (new_frame != -1) {
      //Cargamos la pagina en el frame
      ram_frames[new_frame].pid = pid;
      ram_frames[new_frame].v_page = v_page;
      ram_frames[new_frame].is_page_table = 0;
      ram_frames[new_frame].last_use = demand_paging_timer;
      //Actualizamos tabla de paginas del proceso
      proc->page_table[v_page] = new_frame;
      printf("PAGE IN: PID %d, Page %d loaded into Frame %d\n", pid, v_page, new_frame);
    }
  }
}

/*
 * Punto de entrada del simulador */
void
simulate_demand_paging()
{
  char line[256];
  while (fgets(line, sizeof(line), stdin)) {
    line[strcspn(line, "\r\n")] = 0;
    if (line[0] == '\0' || line[0] == '#') continue;
    int val, pid, pages, v_page;
    if (sscanf(line, "RAM_CONFIG FRAMES %d", &val) == 1)
      init_demand_system(val);
    else if (sscanf(line, "LOAD PID %d PAGES %d", &pid, &pages) == 2)
      load_process(pid, pages);
    else if (sscanf(line, "ACCESS PID %d V_PAGE %d", &pid, &v_page) == 2)
      access_page(pid, v_page);
  }
}
