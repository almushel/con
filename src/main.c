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
	JSON_OBJ,
	JSON_ARR
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

str8 read_all(char* filename) {
	char buf[1024];
	char* data = darr_new(char, sizeof(buf));
	str8 result = {};

	FILE* fstream = fopen(filename, "r");
	if (!fstream) {
		perror("(read_all) fread");
		goto cleanup;
	}

	size_t bytes_read = 0;
	while ((bytes_read = fread(buf, 1, sizeof(buf), fstream)) > 0){
		darr_push_arr(data, buf, bytes_read);
	}

	if (feof(fstream) && !ferror(fstream)) {
		result = new_str8(data, darr_len(data));
	}

	fclose(fstream);

	cleanup:
		darr_free(data);
		return result;
}

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

void push_json_prop(json_obj* map, str8 key, json_node prop) {
	size_t index = str8_hash(key) % map->length;
	assert(index < map->length);

	json_node* dest = map->props + index;
	if (dest->key.length == 0 || strcmp(dest->key.data, key.data) == 0) {
		dest->key = key;
		dest->type = prop.type;
		dest->val = prop.val;
	} else {
		while (dest->next) {
			dest = dest->next;
		}

		darr_push(map->pool, prop);
		dest->next = map->pool + darr_len(map->pool)-1;
	}
}

str8 json_prop_to_str8(json_node prop) {
	str8 result;
	switch (prop.type) {
		case JSON_NULL: {
			result.data = "null";
			result.length = 4;
		} break;

		case JSON_BOOL: {
			result.data = prop.val.boolean ? "true" : "false";
			result.length = 4 + (size_t)(!prop.val.boolean);
		} break;

		case JSON_STRING: {
			result = new_str8("", prop.val.string.length+2);
			result.data[0] = '\"';
			memcpy(result.data+1, prop.val.string.data, prop.val.string.length);
			result.data[result.length-1] = '\"';
		} break;

		case JSON_NUM: {
			int len = snprintf(0,0, "%f", prop.val.number);
			if (len) {
				result = new_str8("", len);	
				snprintf(result.data,result.length+1, "%f", prop.val.number);
			}
		} break;

		default: {} break;
	}

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
	assert(contents[length] == '\0');

	size_t colons = 0;
	for (int c = 0; c < length; c++) {
		if (contents[c] == ',') colons++;	
	}

	json_obj result = new_json_obj(colons);
	str8 cs = str8_trim(
			(str8){.data = contents, .length = length},
			(str8){.data = "{}", .length = 2},
			true
		);
	str8* lines = str8_split(cs, ',');

	str8 vals[2];
	for (int i = 0; i < darr_len(lines); i++) {
		str8_cut(lines[i], ':', vals);

		if (vals[0].length && vals[1].length) {
			str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, false);
			str8 valstr = str8_trim_space(vals[1], true);
			json_node prop = {.key = key};
			double num_val = 0;
			
			if (valstr.data[0] == '\"' && valstr.data[valstr.length-1] == '\"') {
				printf("string\n");
				prop.type = JSON_STRING;
				prop.val.string = str8_trim(valstr, (str8){.data = "\"", .length = 1}, false);
			} else if (strcmp(valstr.data, "true") == 0){
				printf("bool\n");
				prop.type = JSON_BOOL;
				prop.val.boolean = true;
			} else if (strcmp(valstr.data, "false") == 0 ){
				printf("bool\n");
				prop.type = JSON_BOOL;
				prop.val.boolean = false;
			} else if ( (num_val = strtod(valstr.data, 0)) ){
				printf("num\n");
				prop.type = JSON_NUM;
				prop.val.number = num_val;
			} else if (valstr.data[0] == '{' && valstr.data[valstr.length-1] == '}'){
				printf("obj\n");
				prop.type = JSON_STRING; // JSON_OBJ;
				prop.val.string = valstr;
			} else if (valstr.data[0] == '[' && valstr.data[valstr.length-1] == ']'){
				printf("arr\n");
				prop.type = JSON_STRING; // JSON_ARR;
				prop.val.string = valstr;
			}

			push_json_prop(&result, key, prop);
		}
	}
	
	return result;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: con [filename]\n");		
		return 1;
	}

	str8 contents = str8_trim_space(read_all(argv[1]), true);
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
			printf("%s\"%s\" : %s\n", indent, node->key.data, json_prop_to_str8(*node).data);
			node = node->next;
		}
	}
	printf("}\n");

	return 0;
}
