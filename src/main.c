#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "darr.h"

#include "str8.c"


enum json_types {
	JSON_NULL = 0,
	JSON_NUM,
	JSON_STRING,
	JSON_BOOL,
	JSON_OBJ
};

struct json_node;

typedef struct json_obj {
	struct json_node* props;
	size_t length;

	struct json_node* pool;
} json_obj;

typedef union {
	str8 string;
	double number;
	bool boolean;
	void* object;
} json_val;

typedef struct json_node {
	enum json_types type;
	str8 key;
	json_val val;

	struct json_node* next;
} json_node;

size_t str8_hash(str8 key) {
	size_t result = 0;

	for (int i = 0; i < key.length; i++) {
		result += key.data[i] << 1;
	}

	return result;
}

#define STR8_OBJ_MIN 8

json_obj new_json_obj(size_t length) {
	if (length < STR8_OBJ_MIN) {
		length = STR8_OBJ_MIN;
	}

	json_obj result = (json_obj) {
		.props = calloc(length, sizeof(json_node)),
		.length = length,
		.pool = darr_new(json_node, length/2)
	};

	return result;
}

void push_str8_map(json_obj* map, str8 key, json_val val) {
	size_t index = str8_hash(key) % map->length;
	assert(index < map->length);

	json_node* dest = map->props + index;
	if (dest->key.length == 0 || strcmp(dest->key.data, key.data) == 0) {
		dest->key = key;
		dest->val = val;
	} else {
		while (dest->next) {
			dest = dest->next;
		}

		json_node node = (json_node){
			.key = key,
			.val = val
		};
		darr_push(map->pool, node);
		dest->next = map->pool + darr_len(map->pool)-1;
	}
}

str8 read_all(char* filename) {
	char buf[1024];
	str8 result = (str8){
		.data = malloc(sizeof(buf)),
		.length = sizeof(buf),
	};
	int used = 0;

	FILE* fstream = fopen(filename, "r");

	if (!fstream) goto cleanup;

	size_t bytes_read = 0;
	while ((bytes_read = fread(buf, 1, sizeof(buf), fstream)) > 0){
		if (used + bytes_read > result.length) {
			char* new_result = realloc(result.data, result.length * 2);
			if (new_result) {
				result.data = new_result;
				result.length *= 2;
			} else {
				fprintf(stderr, "failed to realloc result\n");
				return result;
			}
		}

		memcpy(result.data+used, buf, bytes_read);
		used += bytes_read;
	}

	fclose(fstream);

	if (!feof(fstream) && ferror(fstream)) { goto cleanup; }

	if (used < result.length) {
		char* new_result = realloc(result.data, used);
		if (new_result) {
			result.data = new_result;
			result.length = used;
		}
	}

	return result;

	cleanup:
		perror("(read_all) fread");
		free(result.data);

		result.data = 0;
		result.length = 0;

		return result;
}

bool validate_json_object(char* contents, size_t length) {
	if (	(contents == 0) || (length == 0) )  {
		fprintf(stderr, "invalid json: empty\n");
		return false;
	}

	// TODO: Verify that characters being skipped are whitespace
	size_t start = 0, end = length;
	while (contents[start] != '{' && start < end) start++;
	while (contents[end] != '}' && end > start) end--;

	if ( start >= end) {
		fprintf(stderr, "invalid json: no top level object\n");
		return false;
	}
	
	return true;
}

json_obj parse_json_object(char* contents, size_t length) {	
	assert(contents[0] == '{');
	assert(contents[length-1] == '}');

	size_t colons = 0;
	for (int c = 0; c < length; c++) {
		if (contents[c] == ',') colons++;	
	}

	json_obj result = new_json_obj(colons);
	str8 cs = str8_trim(
			(str8){.data = contents, .length = length},
			(str8){.data = "{}", .length = 2}
		);
	str8* lines = str8_split(cs, ',');

	str8 vals[2];
	for (int i = 0; i < darr_len(lines); i++) {
		//printf("line: %s\n---\n", lines[i].data);
		str8_cut(lines[i], ':', vals);
		if (vals[0].length && vals[1].length) {
			str8 key = str8_trim_space(vals[0]);
			json_val val = { .string = str8_trim_space(vals[1]) };

			push_str8_map(&result, key, val);
		}
	}
	
	return result;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: con [filename]\n");		
		return 1;
	}

	str8 contents = read_all(argv[1]);
	if (!validate_json_object(contents.data, contents.length)) {
		fprintf(stderr, "%s\n", contents.data);
		return 1;
	}

	json_obj result = parse_json_object(contents.data, contents.length);

	printf("{\n");
	const char* indent = "    ";
	for (int i = 0; i < result.length; i++) {
		json_node* node = result.props + i;
		while (node && node->key.length) {
			printf("%s%s : %s\n", indent, node->key.data, node->val.string.data);
			node = node->next;
		}
	}
	printf("}\n");

	return 0;
}
