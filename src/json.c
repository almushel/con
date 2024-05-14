#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "darr.h"
#include "str8.h"

#define STR8_OBJ_MIN 8

enum json_data_types {
	JSON_NULL = 0,
	JSON_NUM,
	JSON_STRING,
	JSON_BOOL,
	JSON_OBJ,
	JSON_LIST
};

typedef struct json_node json_node;
typedef struct json_val json_val;

typedef struct json_obj {
	json_node* props;
	size_t length;

	json_node* pool;
} json_obj;

struct json_val {
	enum json_data_types type;
	union {
		bool boolean;
		double number;
		str8 string;
		json_obj object;
		json_val* list;
	};
};

struct json_node {
	str8 key;
	json_val val;

	json_node* next;
};

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

void push_json_prop(json_obj* map, str8 key, json_val prop) {
	size_t index = str8_hash(key) % map->length;
	assert(index < map->length);

	json_node* dest = map->props + index;
	if (dest->key.length == 0 || strcmp(dest->key.data, key.data) == 0) {
		dest->key = key;
		dest->val = prop;
	} else {
		while (dest->next) {
			dest = dest->next;
		}


		json_node node = {.key = key, .val = prop};
		darr_push(map->pool, node);
		dest->next = map->pool + darr_len(map->pool)-1;
	}
}

#define INDENT_SIZE 4
str8 json_prop_to_str8(json_node prop, int indent);

str8 json_val_to_str8(json_val val, int indent) {
	str8 result = {};

	switch (val.type) {
		case JSON_NULL: {
			result = new_str8("null", 4);
		} break;

		case JSON_BOOL: {
			result = new_str8(val.boolean ? "true" : "false", 4 + (size_t)(!val.boolean));
		} break;

		case JSON_STRING: {
			result = new_str8("", val.string.length+2);
			result.data[0] = '\"';
			memcpy(result.data+1, val.string.data, val.string.length);
			result.data[result.length-1] = '\"';
		} break;

		case JSON_NUM: {
			int len = snprintf(0,0, "%f", val.number);
			if (len) {
				result = new_str8("", len);
				snprintf(result.data, result.length+1, "%f", val.number);
			}
		} break;

		case JSON_OBJ: {
			result = (str8){.data = "{", .length = 1};
			for (int i = 0; i < val.object.length; i++) {
				if (val.object.props[i].key.length) {
					result = str8_concat(result, (str8){.data="\n", .length = 1});
					result = str8_concat(
						result,
						json_prop_to_str8(val.object.props[i], indent+1)
					);
				}
			}

			if (result.length > 1) {
				str8 bracket = new_str8("", (indent*INDENT_SIZE)+2);
				bracket.data[0] ='\n';
				memset(bracket.data+1, ' ', indent*INDENT_SIZE);
				bracket.data[bracket.length-1] = '}';
				
				result = str8_concat(result, bracket);
			} else {
				result = new_str8("{}", 2);
			}
		} break;

		case JSON_LIST: {
			if (darr_len(val.list)) {
				result = (str8){.data = "[", .length = 1};
				str8 indent_str = new_str8("", (indent+1)*INDENT_SIZE);
				memset(indent_str.data, ' ', indent_str.length);

				json_node node = {};
				for (int i = 0; i < darr_len(val.list); i++) {
					node.val = val.list[i];
					result = str8_concat(result, (str8){.data="\n", .length = 1});
					str8 val_str = json_val_to_str8(node.val, indent+1);
					
					result = str8_concat(result, str8_concat(indent_str, val_str));
				}

				str8 bracket = new_str8("", (indent*INDENT_SIZE)+2);
				bracket.data[0] ='\n';
				memcpy(bracket.data+1, indent_str.data, indent*INDENT_SIZE);
				bracket.data[bracket.length-1] = ']';
				
				result = str8_concat(result, bracket);
			} else {
				result = new_str8("[]", 2);
			}
		} break;

		default: {} break;
	}

	return result;
}

str8 json_prop_to_str8(json_node prop, int indent) {
	str8 key, valstr, result = {};
	int key_offset = indent*INDENT_SIZE;

	key = new_str8("", (key_offset) + (prop.key.length+4));
	memset(key.data, ' ', key_offset);
	key.data[key_offset] = '\"';
	memcpy(key.data+(key_offset+1), prop.key.data, prop.key.length);
	key.data[key.length-3] = '\"';
	key.data[key.length-2] = ':';
	key.data[key.length-1] = ' ';

	valstr = json_val_to_str8(prop.val, indent);

	result = str8_concat(key, valstr);

	return result;
}

bool validate_json_object(char* contents, size_t length) {
	if ( (contents == 0) || (length == 0) )  {
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

json_val parse_json_val(char* contents, size_t length) {
	json_val result = {};
	double num_val = 0;
	
	if (contents[0] == '\"' && contents[length-1] == '\"') {
		result.type = JSON_STRING;
		result.string = str8_trim((str8){.data=contents, .length = length}, (str8){.data = "\"", .length = 1}, false);
	} else if (strcmp(contents, "true") == 0){
		result.type = JSON_BOOL;
		result.boolean = true;
	} else if (strcmp(contents, "false") == 0 ){
		result.type = JSON_BOOL;
		result.boolean = false;
	} else if ( (num_val = strtod(contents, 0)) ){
		result.type = JSON_NUM;
		result.number = num_val;
	}

	return result;
}

json_val* parse_json_list(char* contents, size_t length) {
	json_val* result = 0;	

	if ((contents[0] == '[') && (contents[length-1] == ']') && (contents[length] == '\0') ) {
		result = darr_new(json_val, 2);
		str8 cs = str8_trim(
			(str8){.data = contents, .length = length}, 
			(str8){.data = "[]", .length = 2},
			true
		);
		str8* val_strs = str8_split(cs, ',');
		
		for (int i = 0; i < darr_len(val_strs); i++) {
			if (val_strs[i].length) {
				json_val item = parse_json_val(val_strs[i].data, val_strs[i].length);
				darr_push(result, item);
			}
		}
	}

	return result;
}

json_obj parse_json_object(char* contents, size_t length) {
	if ((contents[0] != '{') || (contents[length-1] != '}') || (contents[length] != '\0')) {
		return (json_obj){};
	}

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

			json_val new_obj = {
				.type = JSON_OBJ,
				.object = parse_json_object(objstr.data, objstr.length),
			};
			push_json_prop(&result, key, new_obj);
			
			i = end+1;
			start = i+1;
		} else if (cs.data[i] == '[') {
			int end;
			for (end = i+1;end < cs.length;end++) {
				if (cs.data[end] == ']') break;
	 		}
				
			str8_cut((str8) {.data = cs.data+start, .length = end-start+1}, ':', vals);
			str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, false);
			str8 liststr = str8_trim_space(vals[1], false);

			json_val new_list = {
				.type = JSON_LIST,
				.list = parse_json_list(liststr.data, liststr.length)
			};
			push_json_prop(&result, key, new_list);
			
			i = end+1;
			start = i+1;
		} else if (cs.data[i] == ',' || i == cs.length-1) {
			str8_cut((str8){.data = cs.data+start, .length = i-start}, ':', vals);

			if (vals[0].length && vals[1].length) {
				str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, false);
				str8 valstr = str8_trim_space(vals[1], true);
				json_val val = parse_json_val(valstr.data, valstr.length);

				push_json_prop(&result, key, val);
			}

			start = i+1;
		}
	}

	
	return result;
}

