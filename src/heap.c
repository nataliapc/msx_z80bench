#include <stdint.h>
#include "heap.h"

#define PUSHSTACK_SIZE 10

static void *_aux;

static void *_pushStack[PUSHSTACK_SIZE];
static const uint8_t _pushIndex = 0;

void *malloc(uint16_t size)
{
	_aux = (void*)heap_top;
	heap_top += size;
	return _aux;
}

void free(uint16_t size)
{
	heap_top -= size;
}

void *heapPush()
{
	if (_pushIndex < PUSHSTACK_SIZE) {
		_pushStack[_pushIndex] = heap_top;
		(*((uint8_t*)&_pushIndex))++;
		return (void*)heap_top;
	} else {
		return (void*)0;
	}
}

void *heapPop()
{
	_aux = heap_top;

	if (_pushIndex > 0) {
		(*((uint8_t*)&_pushIndex))--;
		heap_top = _pushStack[_pushIndex];
		return _aux;
	} else {
		return (void*)0;
	}
}