#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "darr.h"
#include "str8.h"

#define STR8_OBJ_MIN 8

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
	json_obj object;
} json_val;

typedef struct json_node {
	enum json_types type;
	str8 key;
	json_val val;

	struct json_node* next;
} json_node;

static size_t str8_hash(str8 key) {
	size_t result = 0;

	for (int i = 0; i < key.length; i++) {
		result += key.data[i] << 1;
	}

	return result;
}

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

#define INDENT_SIZE 4
str8 json_prop_to_str8(json_node prop, int indent) {
	str8 result = {};

	int key_offset = indent*INDENT_SIZE;
	str8 key = new_str8("", (key_offset) + (prop.key.length+4));
	memset(key.data, ' ', key_offset);

	key.data[key_offset] = '\"';
	memcpy(key.data+(key_offset+1), prop.key.data, prop.key.length);
	key.data[key.length-3] = '\"';
	key.data[key.length-2] = ':';
	key.data[key.length-1] = ' ';

	switch (prop.type) {
		case JSON_NULL: {
			result = str8_concat(key, (str8){.data="null",.length=4});
		} break;

		case JSON_BOOL: {
			result = str8_concat(
				key,
				(str8) {
					.data = prop.val.boolean ? "true" : "false",
					.length = 4 + (size_t)(!prop.val.boolean),
				}
			);
				
		} break;

		case JSON_STRING: {
			result = new_str8("", prop.val.string.length+2);
			result.data[0] = '\"';
			memcpy(result.data+1, prop.val.string.data, prop.val.string.length);
			result.data[result.length-1] = '\"';

			result = str8_concat(key, result);
		} break;

		case JSON_NUM: {
			int len = snprintf(0,0, "%f", prop.val.number);
			if (len) {
				result = new_str8("", len);
				snprintf(result.data, result.length+1, "%f", prop.val.number);
			}
			result = str8_concat(key, result);
		} break;

		case JSON_OBJ: {
			result = (str8){.data = "{", .length = 1};
			for (int i = 0; i < prop.val.object.length; i++) {
				if (prop.val.object.props[i].key.length) {
					result = str8_concat(result, (str8){.data="\n", .length = 1});
					result = str8_concat(
						result,
						json_prop_to_str8(prop.val.object.props[i], indent+1)
					);
				}
			}

			str8 bracket = new_str8("", (indent*INDENT_SIZE)+2);
			bracket.data[0] ='\n';
			memset(bracket.data+1, ' ', indent*INDENT_SIZE);
			bracket.data[bracket.length-1] = '}';
			
			result = str8_concat(result, bracket);
			result = str8_concat(key, result);
		}

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

	json_obj result = new_json_obj(16);
	str8 cs = str8_trim(
			(str8){.data = contents, .length = length},
			(str8){.data = "{}", .length = 2},
			true
	);

	int start = 0;
	str8 vals[2];
	for (int i = 0; i < cs.length; i++) {
		if (cs.data[i] == '{') {
			int end;
			for (end = i+1;end < cs.length;end++) {
				if (cs.data[end] == '}') break;
	 		}
				
			str8_cut((str8) {.data = cs.data+start, .length = end-start+1}, ':', vals);
			str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, false);
			str8 objstr = str8_trim_space(vals[1], false);

			json_node prop = {
				.key = key,
				.type = JSON_OBJ,
				.val.object = parse_json_object(objstr.data, objstr.length),
			};
			push_json_prop(&result, key, prop);
			
			i = end+1;
			start = i+1;
		} else if (cs.data[i] == ',' || i == cs.length-1) {
			str8_cut((str8){.data = cs.data+start, .length = i-start}, ':', vals);

//			printf("key: %s, val: %s\n", vals[0].data, vals[1].data);

			if (vals[0].length && vals[1].length) {
				str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, false);
				str8 valstr = str8_trim_space(vals[1], true);
				json_node prop = {.key = key};
				double num_val = 0;
				
				if (valstr.data[0] == '\"' && valstr.data[valstr.length-1] == '\"') {
					prop.type = JSON_STRING;
					prop.val.string = str8_trim(valstr, (str8){.data = "\"", .length = 1}, false);
				} else if (strcmp(valstr.data, "true") == 0){
					prop.type = JSON_BOOL;
					prop.val.boolean = true;
				} else if (strcmp(valstr.data, "false") == 0 ){
					prop.type = JSON_BOOL;
					prop.val.boolean = false;
				} else if ( (num_val = strtod(valstr.data, 0)) ){
					prop.type = JSON_NUM;
					prop.val.number = num_val;
				}

				push_json_prop(&result, key, prop);
			}

			start = i+1;
		}
	}

	
	return result;
}

