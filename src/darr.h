// Dynamic array

#ifndef DARR_H
#define DARR_H

#include <assert.h>
#include <stdlib.h>

//NOTE: Probably trying to be too clever here!

#define darr_head(arr) ((size_t*)arr - 2)
#define darr_free(arr) free(darr_head(arr))
#define darr_len(arr) *((size_t*)arr - 1)
#define darr_cap(arr) *(darr_head(arr))

#define darr_new(arr, type, cap) {\
	arr = (type*)( (size_t*)malloc(sizeof(type)*cap + sizeof(size_t)*2) + 2 );\
	darr_cap(arr) = cap;\
	darr_len(arr) = 0;\
}

#define darr_resize(arr, new_cap) {\
	size_t* new_head = realloc(darr_head(arr), sizeof(arr[0])*new_cap + sizeof(size_t)*2);\
	assert(new_head);\
	arr = (void*)(new_head+2);\
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
