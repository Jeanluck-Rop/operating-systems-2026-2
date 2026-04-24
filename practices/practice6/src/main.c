#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm_bl.h"
#include "mmu.h"
#include "paging_basic.h"
#include "paging_on_demand.h"

#define STUDENT_ID 118005762

// External initializers
void init_mmu();
void enable_tlb();

void print_help() {
    printf("Usage: c_sim [MODE] [-i input_file] [-o output_file]\n");
    printf("Modes:\n");
    printf("  --vm-bl          Virtual Memory Base-Limit Simulation\n");
    printf("  --basic-paging   Basic Paging Simulation\n");
    printf("  --mmu-basic      Basic MMU Address Translation\n");
    printf("  --tlb            MMU with TLB (LRU/FIFO Policy)\n");
    printf("  --demand-paging  Demand Paging Simulation\n");
    printf("General Flags:\n");
    printf("  -i <file>        Input file (default: stdin)\n");
    printf("  -o <file>        Output file (default: stdout)\n");
}

int main(int argc, char *argv[]) {
    printf("[ID:%d]\n", STUDENT_ID);

    int mode_bl = 0;
    int mode_paging = 0;
    int mode_mmu = 0;
    int mode_tlb = 0;
    int mode_demand = 0;

    init_mmu();

    // First pass: find modes and I/O redirection
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--vm-bl") == 0) {
            mode_bl = 1;
        } else if (strcmp(argv[i], "--basic-paging") == 0) {
            mode_paging = 1;
        } else if (strcmp(argv[i], "--mmu-basic") == 0) {
            mode_mmu = 1;
        } else if (strcmp(argv[i], "--tlb") == 0) {
            mode_tlb = 1;
            mode_mmu = 1;
            enable_tlb();
        } else if (strcmp(argv[i], "--demand-paging") == 0) {
            mode_demand = 1;
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            if (freopen(argv[++i], "r", stdin) == NULL) {
                perror("Error opening input file");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            if (freopen(argv[++i], "w", stdout) == NULL) {
                perror("Error opening output file");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
    }

    if (mode_bl) {
        char line[256];
        while (fgets(line, sizeof(line), stdin)) {
            line[strcspn(line, "\r\n")] = 0;
            if (line[0] == '\0' || line[0] == '#') continue;

            int pid, base, limit, addr, val;
            if (sscanf(line, "ASSIGN PID %d BASE %d LIMIT %d", &pid, &base, &limit) == 3) {
                assign_process(pid, base, limit);
                printf("SYSTEM (BL): Process %d configured with Base: 0x%03X, Limit: %d bytes\n", pid, base, limit);
            } else if (sscanf(line, "WRITE PID %d ADDR %d VAL %d", &pid, &addr, &val) == 3) {
                ram_write(pid, addr, (unsigned char)val);
            } else if (sscanf(line, "READ PID %d ADDR %d", &pid, &addr) == 2) {
                ram_read(pid, addr);
            }
        }
    } else if (mode_paging) {
        simulate_basic_paging();
    } else if (mode_mmu) {
        char line[256];
        int page_size = 0;
        while (fgets(line, sizeof(line), stdin)) {
            line[strcspn(line, "\r\n")] = 0;
            if (line[0] == '\0' || line[0] == '#') continue;

            int p, f, v_addr;
            if (sscanf(line, "PAGE_SIZE %d", &page_size) == 1) {
                printf("SYSTEM: Page Size set to %d\n", page_size);
            } else if (sscanf(line, "PAGE_TABLE SET PAGE %d FRAME %d", &p, &f) == 2) {
                set_page_table_entry(p, f);
            } else if (sscanf(line, "TRANSLATE ADDR %d", &v_addr) == 1) {
                mmu_translate(v_addr, page_size);
            }
        }
    } else if (mode_demand) {
        simulate_demand_paging();
    } else {
        printf("Error: No simulation mode specified. Use --vm-bl, --basic-paging, --mmu-basic, --tlb, or --demand-paging.\n");
        print_help();
    }

    return 0;
}
