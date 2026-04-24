#include <stdio.h>
#include "tlb.h"

tlb_entry_t tlb[TLB_SIZE]; //Arreglo de entradas del TLB (cache de traducciones)
int global_timer = 0; //Reloj simulado para LRU/FIFO

/* Inicializa todas las entradas del TLB como invalidas */
void
init_tlb()
{
  for (int i = 0; i < TLB_SIZE; i++) {
    tlb[i].valid = 0;
    tlb[i].page = -1;
    tlb[i].frame = -1;
    tlb[i].last_access = 0;
    tlb[i].insertion_time = 0;
  }
}

/*
 * Busca una pagina en el TLB.
 * Recorre todas las entradas del TLB buscando una entrada valida
 * que coincida con el numero de pagina solicitado.
 */
int
tlb_lookup(int page)
{
  global_timer++;
  for (int i = 0; i < TLB_SIZE; i++) {
    if (tlb[i].valid && tlb[i].page == page) {
      tlb[i].last_access = global_timer;  //Actualizamos para politica LRU
      printf("TLB HIT: Page %d found in Entry %d (Frame %d)\n", page, i, tlb[i].frame);
      return tlb[i].frame;
    }
  }
  
  // Si no se encuentra (MISS), notificar y retornar -1
  printf("TLB MISS: Page %d not in TLB\n", page);
  return -1;
}


/*
 * Inserta una traduccion pagina->frame en el TLB
 * Estrategia de insercion:
 *   1. Buscamos una entrada vacia (valid == 0) y se inserta ahi.
 *   2. Si el TLB esta lleno, aplicamos desalojo LRU:
 *      - Seleccionamos la entrada con menor last_access (menos recientemente usada).
 *      - En caso de empate en last_access, usamos FIFO como desempate,
 *        desalojamos la que fue insertada primero (menor insertion_time).
 */
void
tlb_insert(int page,
	   int frame)
{
  global_timer++;
    
  //Caso 1: Buscamos entrada vacia para insertar sin desalojar
  for (int i = 0; i < TLB_SIZE; i++) {
    if (!tlb[i].valid) {
      tlb[i].valid = 1;
      tlb[i].page = page;
      tlb[i].frame = frame;
      tlb[i].last_access = global_timer;
      tlb[i].insertion_time = global_timer;
      printf("TLB INSERT: Page %d -> Frame %d (Entry %d)\n", page, frame, i);
      return;
    }
  }

  //Caso 2: TLB lleno, seleccionamos victima con LRU + FIFO como desempate
  int victim = 0;
  for (int i = 1; i < TLB_SIZE; i++) {
    if (tlb[i].last_access < tlb[victim].last_access)
      victim = i;
    else if (tlb[i].last_access == tlb[victim].last_access)
      //Empate en LRU: desalojar el que se inserto primero (FIFO)
      if (tlb[i].insertion_time < tlb[victim].insertion_time)
	victim = i;
  }
  printf("TLB EVICT: Entry %d (Page %d) replaced\n", victim, tlb[victim].page);
  
  //Sobreescribimos la entrada victima con la nueva traduccion
  tlb[victim].valid = 1;
  tlb[victim].page = page;
  tlb[victim].frame = frame;
  tlb[victim].last_access = global_timer;
  tlb[victim].insertion_time = global_timer;
  printf("TLB INSERT: Page %d -> Frame %d (Entry %d)\n", page, frame, victim);
}
