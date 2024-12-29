
#ifndef  __HEAP_MSXDOS_H__
#define  __HEAP_MSXDOS_H__

#ifdef __HEAP_H__
#error You cannot use both MSXDOS and non-DOS heap functions
#endif

#include <stdint.h>


extern uint8_t *heap_top;

void *malloc(uint16_t size);
void free(void *ptr);
void freeSize(uint16_t size);

void *heapPush();
void *heapPop();


#endif//__HEAP_MSXDOS_H__
