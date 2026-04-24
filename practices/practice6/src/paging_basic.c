#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "paging_basic.h"

/* Simulador de paginacion basica.
 * Leemos comandos desde stdin y gestionamos la asignacion de marcos (frames) a procesos
 */
void
simulate_basic_paging()
{
  int page_size = 0;    //Tamano de cada pagina/marco en bytes
  int ram_size = 0;     //Tamano total de la RAM en bytes
  int total_frames = 0; //Numero total de marcos en el sistema
  int free_frames = 0;  //Marcos actualmente disponibles
  char line[256];
  
  while (fgets(line, sizeof(line), stdin)) {
    line[strcspn(line, "\r\n")] = 0;
    if (line[0] == '\0' || line[0] == '#')
      continue;
    
    if (strncmp(line, "PAGE_SIZE", 9) == 0) {
      sscanf(line, "PAGE_SIZE %d", &page_size);
      printf("SYSTEM: Page Size set to %d bytes\n", page_size);
    } else if (strncmp(line, "RAM_SIZE", 8) == 0) {
      sscanf(line, "RAM_SIZE %d", &ram_size);
      //Solo se cuentan marcos completos
      total_frames = ram_size / page_size;
      free_frames = total_frames;
      printf("SYSTEM: RAM Size set to %d bytes (%d frames)\n", ram_size, total_frames);
    } else if (strncmp(line, "PROCESS PID", 11) == 0) {
      int pid;
      int size;
      if (sscanf(line, "PROCESS PID %d SIZE %d", &pid, &size) == 2) {
	int pages_needed = (size + page_size - 1) / page_size;
	if (pages_needed <= free_frames) {
	  // Actualizar frames libres
	  free_frames -= pages_needed;
	  printf("PROCESS: PID %d (Size: %d) -> %d pages allocated. Frames left: %d\n",
		 pid, size, pages_needed, free_frames);
	} else {
	  // Notificar falla por falta de memoria
	  printf("PROCESS: PID %d (Size: %d) -> %d pages needed. FAILED (Not enough memory, frames left: %d)\n", 
		 pid, size, pages_needed, free_frames);
	}
      }
    }
  }
}
