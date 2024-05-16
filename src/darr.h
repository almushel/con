// Dynamic array

#ifndef DARR_H
#define DARR_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//NOTE: Probably trying to be too clever here!

typedef struct {
	size_t capacity, size;
} darr_header;

#define DARR_DEFAULT_CAP 16

#define darr_def(type) \
typedef struct type##_list {\
	type* data;\
	size_t length, capacity;\
} type##_list

#define darr_resize(arr, size, new_cap) {\
	arr.data = realloc(arr.data, size*(new_cap > 0 ? new_cap : DARR_DEFAULT_CAP));\
	assert(arr.data);\
	arr.capacity = new_cap > 0 ? new_cap : DARR_DEFAULT_CAP;\
	if (arr.length > arr.capacity) { arr.length = arr.capacity; }\
};

#define darr_push(arr, val) {\
	if (arr.length >= arr.capacity) darr_resize(arr, sizeof(val), arr.capacity * 2)\
	arr.data[arr.length] = val;\
	arr.length++;\
}

#define darr_push_arr(arr, parr, plen) {\
	while (arr.length+plen >= arr.capacity)\
		darr_resize(arr, sizeof(parr[0]), arr.capacity*2);\
	memcpy(arr.data+arr.length, parr, sizeof(parr[0])*plen);\
	arr.length += plen;\
}

#endif
