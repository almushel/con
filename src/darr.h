// Dynamic array

#ifndef DARR_H
#define DARR_H

#include <assert.h>

#define darr_head(arr) ((size_t*)arr - 2)
#define darr_free(arr) free((size_t*)arr - 1))
#define darr_len(arr) *((size_t*)arr - 1)
#define darr_size(arr) *(darr_head(arr))

#define darr_new(arr, type, size) {\
	arr = (type*)( (size_t*)malloc(sizeof(type) + sizeof(size_t)*2) + 2 );\
	darr_size(arr) = size;\
	darr_len(arr) = 0;\
}

#define darr_resize(arr, new_size) {\
	arr = realloc(darr_head(arr), new_size);\
	assert(arr);\
	darr_size(arr) = new_size;\
}

#define darr_set_len(arr, new_len) { \
	if (new_len > darr_size(arr)) {\
		darr_resize(arr, new_len);\
	}\
	darr_len(arr) = new_len;\
}

#define darr_push(arr, val) {\
	if (darr_len(arr) >= darr_size(arr)) darr_resize(arr, darr_size(arr) * 2)\
	arr[darr_len(arr)] = val;\
	darr_len(arr) = darr_len(arr)+1;\
}

#endif
