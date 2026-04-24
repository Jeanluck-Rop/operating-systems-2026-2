#ifndef MMU_H
#define MMU_H

void set_page_table_entry(int page, int frame);
int mmu_translate(int virtual_addr, int page_size);

#endif // MMU_H
