#ifndef TLB_H
#define TLB_H

#define TLB_SIZE 4

typedef struct {
    int page;
    int frame;
    int last_access;   // For LRU
    int insertion_time; // For FIFO (tie-breaker)
    int valid;
} tlb_entry_t;

void init_tlb();
int tlb_lookup(int page);
void tlb_insert(int page, int frame);

#endif // TLB_H
