#pragma once
#include <stdlib.h>

typedef struct {
	void* memory;
	size_t elementCount;
	size_t elementSize;
	size_t capacity;
} ArrayList;

ArrayList ArrayList_new(size_t elementCount, size_t elementSize);
void ArrayList_resize(ArrayList* arrayList, size_t newElementCount);
void ArrayList_add(ArrayList* arrayList, void* element);
void ArrayList_free(ArrayList* arrayList);