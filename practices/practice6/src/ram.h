#ifndef RAM_H
#define RAM_H

#define RAM_SIZE 1024

typedef struct {
    int pid;
    int base;
    int limit;
} process_t;

extern unsigned char ram[RAM_SIZE];

#endif // RAM_H
