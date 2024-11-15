#include "arraylist.h"

#include <string.h>
#include <assert.h>

ArrayList ArrayList_new(size_t elementCount, size_t elementSize) {
	ArrayList result;
	result.capacity = elementCount;
	result.elementCount = elementCount;
	result.elementSize = elementSize;
	if (elementCount > 0) {
		result.memory = malloc(elementCount * elementSize);
	} else {
		result.memory = NULL;
	}
	return result;
}

void ArrayList_resize(ArrayList* arrayList, size_t newElementCount) {
	assert(arrayList->elementSize > 0 && "You probably forgot to initialize the array elementSize should be more than 0");
	if (newElementCount == 0) {
		ArrayList_free(arrayList);
		return;
	}
	if (newElementCount > arrayList->capacity) {
		arrayList->capacity = (newElementCount > arrayList->capacity ? newElementCount : arrayList->capacity) * 2;
		arrayList->memory = realloc(arrayList->memory, arrayList->capacity * arrayList->elementSize);
	}
	arrayList->elementCount = newElementCount;
}

void ArrayList_add(ArrayList* arrayList, void* element) {
	ArrayList_resize(arrayList, arrayList->elementCount+1);
	void* memory = (void*)((intptr_t)arrayList->memory + (arrayList->elementCount - 1) * arrayList->elementSize);
	memcpy(memory, element, arrayList->elementSize);
}

void ArrayList_free(ArrayList* arrayList) {
	free(arrayList->memory);
	arrayList->memory = NULL;
	arrayList->elementCount = 0;
	arrayList->capacity = 0;
}