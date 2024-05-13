// Dynamic array

#ifndef DARR_H
#define DARR_H

#include <assert.h>
#include <stdlib.h>

//NOTE: Probably trying to be too clever here!

typedef struct {
	size_t capacity, length;
} darr_header;

#define darr_head(arr) ((darr_header*)arr - 1)
#define darr_free(arr) free(darr_head(arr))
#define darr_len(arr) darr_head(arr)->length
#define darr_cap(arr) darr_head(arr)->capacity

static inline void* darr_alloc(size_t size, size_t cap) {
	darr_header* result = (darr_header*)malloc(size*cap + sizeof(darr_header));
	result->capacity = cap;
	result->length = 0;

	return (void*)(result+1);
}

#define darr_new(type, cap) darr_alloc(sizeof(type), cap)

#define darr_resize(arr, new_cap) {\
	darr_header* new_head = realloc(darr_head(arr), sizeof(arr[0])*new_cap + sizeof(darr_header));\
	assert(new_head);\
	arr = (void*)(new_head+1);\
	darr_cap(arr) = new_cap;\
}

#define darr_set_len(arr, new_len) { \
	if (new_len > darr_cap(arr)) {\
		darr_resize(arr, new_len);\
	}\
	darr_len(arr) = new_len;\
}

//NOTE: Maybe use memcpy here?
#define darr_push(arr, val) {\
	if (darr_len(arr) >= darr_cap(arr)) darr_resize(arr, darr_cap(arr) * 2)\
	arr[darr_len(arr)] = val;\
	darr_len(arr) = darr_len(arr)+1;\
}

#define darr_push_arr(arr, parr, plen) for (int i = 0; i < plen; i ++) { darr_push(arr, parr[i]);}

#endif
