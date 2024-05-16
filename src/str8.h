#ifndef STR8_H
#define STR8_H

#include <stddef.h>

#include "darr.h"

typedef struct str8 {
	char* data;
	size_t length;
} str8;

darr_def(str8);

void str8_free_pool();
str8 new_str8(const char* data, size_t length);
str8 str8_cpy(const str8 src);
bool str8_contains(str8 s, int c);
str8 str8_concat(str8 left, str8 right);
str8_list str8_split(str8 s, int delim);
str8* str8_cut(str8 s, int sep, str8 result[2]);
str8 str8_trim_space(str8 s, bool inplace);
str8 str8_trim(str8 s, str8 cutset, bool inplace);

#endif

#ifdef STR8_IMPLEMENTATION

#include <ctype.h>

#define POOL_MIN_SIZE 256

typedef struct str8_pool {
	char* data;
	size_t length, capacity;
} str8_pool;
static struct str8_pool pool = {};

void str8_free_pool() {
	if (pool.data) {
		free(pool.data);
		pool = (str8_pool){};
	}
}

str8 new_str8(const char* data, size_t length) {
	int start = pool.length;
	darr_push_arr(pool, data, length);
	darr_push(pool, '\0');

	str8 result = {
		.data = pool.data+start,
		.length = length,
	};

	return result;

}

str8 str8_cpy(const str8 src) {
	str8 result = new_str8(src.data, src.length);
	return result;
}

bool str8_contains(str8 s, int c) {
	for (int i = 0; i < s.length; i++) {
		if (s.data[i] == c) {
			return true;
		}
	}

	return false;
}

str8 str8_concat(str8 left, str8 right) {
	if (left.length == 0) {
		return new_str8(right.data, right.length);
	}
	if (right.length == 0) {
		return new_str8(left.data, left.length);
	}

	str8 result = new_str8(left.data, left.length+right.length);
	memcpy(result.data+left.length, right.data, right.length);

	return result;
}

str8_list str8_split(str8 s, int delim) {
	str8_list result = {};

	int start = 0, c = 0;
	for (; c < s.length; c++) {
		if (s.data[c] == delim) {
			darr_push(result, new_str8(s.data+start, c-start));
			start = c+1;
		}
	}

	if (result.length == 0) {
		darr_push(result, s);
	} else if (start < c) {
		darr_push(result, new_str8(s.data+start, c-start));
	}

	return result;
}

str8* str8_cut(str8 s, int sep, str8 result[2]) {
	result[0] = (str8){};
	result[1] = (str8){};

	for (int i = 0; i < s.length; i++) {
		if (s.data[i] == sep) {
			result[0] = new_str8(s.data, i);
			result[1] = new_str8(s.data+i+1, s.length-i-1);
			break;
		};
	}

	return result;
}

str8 str8_trim_space(str8 s, bool inplace) {
	int start = 0, end = s.length-1;

	while(isspace(s.data[start]) && start < end) {
		start++;
	}

	while(isspace(s.data[end]) && end > start) {
		end--;
	}

	str8 result = {};
	if (start < end) {
		result = inplace
			? (str8){.data = s.data+start, .length = (end-start)+1}
			: new_str8(s.data+start, (end-start)+1);
		result.data[result.length] = '\0';
	}

	return result;
}

str8 str8_trim(str8 s, str8 cutset, bool inplace) {
	int start = 0, end = s.length-1;

	while (str8_contains(cutset, s.data[start]) && start < end) {
		start++;
	}

	while (str8_contains(cutset, s.data[end]) && end > start) {
		end--;
	}

	str8 result = {};
	if (start < end) {
		result = inplace
			? (str8){.data = s.data+start, .length = (end-start)+1}
			: new_str8(s.data+start, (end-start)+1);
		result.data[result.length] = '\0';
	}

	return result;
}

#endif
