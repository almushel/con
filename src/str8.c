#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "darr.h"

typedef struct str8 {
	char* data;
	size_t length;
} str8;

#define POOL_MIN_SIZE 256

static char* pool = 0;

void str8_free_pool() {
	if (pool) {
		darr_free(pool);
	}
}

str8 new_str8(const char* data, size_t length) {
	if (pool == 0) {
		pool = darr_new(char, POOL_MIN_SIZE);
	}

	int start = darr_len(pool);
	darr_push_arr(pool, data, length);
	darr_push(pool, '\0');

	str8 result = {
		.data = pool+start,
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

// NOTE: Do I want to copy or split in place?
str8* str8_split(str8 s, int delim) {
	str8* result = darr_new(str8, 8);

	int start = 0, c = 0;
	for (; c < s.length; c++) {
		if (s.data[c] == delim) {
			darr_push(result, new_str8(s.data+start, c-start));
			start = c+1;
		}
	}

	if (start < c) {
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
			result[1] = new_str8(s.data+i+1, s.length-i);
		};
	}

	return result;
}

str8 str8_trim_space(str8 s) {
	int start = 0, end = s.length-1;

	while(isspace(s.data[start]) && start < s.length) {
		start++;
	}

	while(isspace(s.data[end]) && end > start) {
		end--;
	}

	str8 result = {};
	if (start < end) {
		result = new_str8(s.data+start, (end-start)+1);
	}

	return result;
}

str8 str8_trim(str8 s, str8 cutset) {
	int start = 0, end = s.length-1;

	while (str8_contains(cutset, s.data[start]) && start < s.length) {
		start++;
	}

	while (str8_contains(cutset, s.data[end]) && end > start) {
		end--;
	}

	str8 result = {};
	if (start < end) {
		result = new_str8(s.data+start, end-start);
	}

	return result;	
}
