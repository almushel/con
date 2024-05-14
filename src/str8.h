#ifndef STR8_H
#define STR8_H

#include <stddef.h>

typedef struct str8 {
	char* data;
	size_t length;
} str8;

void str8_free_pool();
str8 new_str8(const char* data, size_t length);
str8 str8_cpy(const str8 src);
bool str8_contains(str8 s, int c);
str8 str8_concat(str8 left, str8 right);
str8* str8_split(str8 s, int delim);
str8* str8_cut(str8 s, int sep, str8 result[2]);
str8 str8_trim_space(str8 s, bool inplace);
str8 str8_trim(str8 s, str8 cutset, bool inplace);

#endif
