#ifndef FREDC_H
#define FREDC_H

#include <stdbool.h>
#include <stddef.h>

#include "darr.h"
#include "str8.h"

enum fredc_data_types {
	JSON_NULL = 0,
	JSON_NUM,
	JSON_STRING,
	JSON_BOOL,
	JSON_OBJ,
	JSON_LIST
};

typedef struct fredc_node fredc_node;
typedef struct fredc_val fredc_val;
typedef struct fredc_obj fredc_obj;
darr_def(fredc_val);
typedef fredc_val_list fredc_list;
darr_def(fredc_node);

struct fredc_obj {
	fredc_node* props; // hash map
	size_t length;

	fredc_node_list pool; // darr (dynamic array)
};

struct fredc_val {
	enum fredc_data_types type;
	union {
		bool boolean;
		double number;
		str8 string;
		fredc_obj object;
		fredc_list list;
	};
};

struct fredc_node {
	str8 key;
	fredc_val val;

	fredc_node* next;
};

fredc_obj new_fredc_obj(size_t length);
bool fredc_validate_json(char* contents, size_t length);
fredc_obj fredc_parse_obj_str(char* contents, size_t length);
fredc_list fredc_parse_list_str(char* contents, size_t length);

str8 fredc_val_str8ify(fredc_val val, int indent);
str8 fredc_node_str8ify(fredc_node prop, int indent);
str8 fredc_obj_str8ify(fredc_obj o);
char* fredc_obj_stringify(fredc_obj o);

void fredc_val_free(fredc_val *v);
void fredc_node_free(fredc_node *n);
void fredc_obj_free(fredc_obj* o);

#endif

#define FREDC_IMPLEMENTATION
#ifdef FREDC_IMPLEMENTATION

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FREDC_OBJ_MIN 8

static size_t str8_hash(str8 key) {
	size_t result = 0;

	for (int i = 0; i < key.length; i++) {
		result += key.data[i] << 1;
	}

	return result;
}

fredc_obj new_fredc_obj(size_t length) {
	if (length < FREDC_OBJ_MIN) {
		length = FREDC_OBJ_MIN;
	}

	fredc_obj result = (fredc_obj) {
		.props = (fredc_node*)calloc(length, sizeof(fredc_node)),
		.length = length,
	};

	return result;
}

void push_fredc_prop(fredc_obj* map, str8 key, fredc_val prop) {
	size_t index = str8_hash(key) % map->length;
	assert(index < map->length);

	str8 key2;
	key2.length = key.length;
	key2.data = (char*)malloc(key2.length+1);
	memcpy(key2.data, key.data, key2.length);

	fredc_node* dest = map->props + index;
	if (dest->key.length == 0 || strcmp(dest->key.data, key2.data) == 0) {
		dest->key = key2;
		dest->val = prop;
	} else {
		while (dest->next) {
			dest = dest->next;
		}

		fredc_node node = {.key = key2, .val = prop};
		darr_push(map->pool, node);
		dest->next = map->pool.data + (map->pool.length-1);
	}
}

#define INDENT_SIZE 4

str8 fredc_val_str8ify(fredc_val val, int indent) {
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
			int count = 0;
			str8 props = {.data = "{", .length = 1};
			for (int i = 0; i < val.object.length; i++) {
				if (val.object.props[i].key.length) {
					props = str8_concat(props, (str8){.data="\n", .length = 1});
					props = str8_concat(
						props,
						fredc_node_str8ify(val.object.props[i], indent+1)
					);
					props = str8_concat(props, (str8){.data = ",", .length = 1});
					count++;
				}
			}

			if (props.length) {
				str8 bracket;
				if (count > 1) {
					props.length--; //Remove last comma

					str8 bracket = new_str8("", (indent*INDENT_SIZE)+2);
					bracket.data[0] ='\n';
					memset(bracket.data+1, ' ', indent*INDENT_SIZE);
					bracket.data[bracket.length-1] = '}';
				} else {
					int offset = (indent+1)*INDENT_SIZE;
					// Remove indent and trailing comma
					props.data += offset;
					props.data[0] = '{';
					props.length -= offset+1;

					bracket.data = " }";
					bracket.length = 2;
				}
				
				result = str8_concat(props, bracket);
			} else {
				result = new_str8("{}", 2);
			}
		} break;

		case JSON_LIST: {
			if (val.list.length) {
				result = (str8){.data = "[", .length = 1};
				str8 indent_str = new_str8("", (indent+1)*INDENT_SIZE);
				memset(indent_str.data, ' ', indent_str.length);

				fredc_node node = {};
				for (int i = 0; i < val.list.length; i++) {
					node.val = val.list.data[i];
					result = str8_concat(result, (str8){.data="\n", .length = 1});
					str8 val_str = fredc_val_str8ify(node.val, indent+1);
					
					result = str8_concat(result, str8_concat(indent_str, val_str));
					if (i != (val.list.length)-1) {
						result = str8_concat(result, (str8){.data = ",", .length = 1});
					}
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

str8 fredc_node_str8ify(fredc_node prop, int indent) {
	str8 key, valstr, result = {};
	int key_offset = indent*INDENT_SIZE;

	key = new_str8("", (key_offset) + (prop.key.length+4));
	memset(key.data, ' ', key_offset);
	key.data[key_offset] = '\"';
	memcpy(key.data+(key_offset+1), prop.key.data, prop.key.length);
	key.data[key.length-3] = '\"';
	key.data[key.length-2] = ':';
	key.data[key.length-1] = ' ';

	valstr = fredc_val_str8ify(prop.val, indent);

	result = str8_concat(key, valstr);

	return result;
}

str8 fredc_obj_str8ify(fredc_obj o) {
	str8 result = fredc_val_str8ify((fredc_val) {.type = JSON_OBJ, .object = o}, 0);
	return result;
}

char* fredc_obj_stringify(fredc_obj o) {
	str8 result = fredc_val_str8ify((fredc_val) {.type = JSON_OBJ, .object = o}, 0);
	return result.data;
}

fredc_val parse_fredc_val(char* contents, size_t length) {
	fredc_val result = {};
	double num_val = 0;
	
	if (contents[0] == '\"' && contents[length-1] == '\"') {
		str8 val = str8_trim((str8){.data=contents, .length = length}, (str8){.data = "\"", .length = 1}, true);

		result.type = JSON_STRING;
		result.string.length = val.length;
		result.string.data = (char*)malloc(val.length+1);
		memcpy(result.string.data, val.data, val.length);
		result.string.data[val.length] = '\0';
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

fredc_val_list fredc_parse_list_str(char* contents, size_t length) {
	fredc_list result = {};	

	if ((contents[0] == '[') && (contents[length-1] == ']') && (contents[length] == '\0') ) {
		str8 cs = str8_trim(
			(str8){.data = contents, .length = length}, 
			(str8){.data = "[]", .length = 2},
			true
		);
		str8_list val_strs = str8_split(cs, ',');
		
		for (int i = 0; i < val_strs.length; i++) {
			if (val_strs.data[i].length) {
				val_strs.data[i] = str8_trim_space(val_strs.data[i], true);	
				fredc_val item = parse_fredc_val(val_strs.data[i].data, val_strs.data[i].length);
				darr_push(result, item);
			}
		}
	}

	return result;
}

fredc_obj fredc_parse_obj_str(char* contents, size_t length) {
	if ((contents[0] != '{') || (contents[length-1] != '}') || (contents[length] != '\0')) {
		return (fredc_obj){};
	}

	fredc_obj result = new_fredc_obj(16);
	str8 cs = str8_trim(
			(str8){.data = contents, .length = length},
			(str8){.data = "{}", .length = 2},
			true
	);

	int start = 0;
	str8 vals[2];
	for (int i = 0; i < cs.length; i++) {
		if (cs.data[i] == '{') {
			int nesting_level = 1;
			int end;
			for (end = i+1;end < cs.length;end++) {
				if (cs.data[end] == '{') nesting_level++; 
				else if (cs.data[end] == '}') {
					nesting_level--;
					if (nesting_level == 0) {
						break;
					}
				}
	 		}
				
			str8_cut((str8) {.data = cs.data+start, .length = end-start+1}, ':', vals);
			str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, true);
			str8 objstr = str8_trim_space(vals[1], true);

			fredc_val new_obj = {
				.type = JSON_OBJ,
				.object = fredc_parse_obj_str(objstr.data, objstr.length),
			};
			push_fredc_prop(&result, key, new_obj);
			
			i = end+1;
			start = i+1;
		} else if (cs.data[i] == '[') {
			int nesting_level = 1;
			int end;
			for (end = i+1;end < cs.length;end++) {
				if (cs.data[end] == '[') nesting_level++;
				else if (cs.data[end] == ']') {
					nesting_level--;
					if (nesting_level == 0) {
						break;
					}
				}
	 		}
				
			str8_cut((str8) {.data = cs.data+start, .length = end-start+1}, ':', vals);
			str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, true);
			str8 liststr = str8_trim_space(vals[1], true);

			fredc_val new_list = {
				.type = JSON_LIST,
				.list = fredc_parse_list_str(liststr.data, liststr.length)
			};
			push_fredc_prop(&result, key, new_list);
			
			i = end+1;
			start = i+1;
		} else if (cs.data[i] == ',' || i == cs.length-1) {
			str8_cut((str8){.data = cs.data+start, .length = i-start}, ':', vals);

			if (vals[0].length && vals[1].length) {
				str8 key = str8_trim(str8_trim_space(vals[0], true), (str8){.data = "\"", .length = 1}, true);
				str8 valstr = str8_trim_space(vals[1], true);
				fredc_val val = parse_fredc_val(valstr.data, valstr.length);

				push_fredc_prop(&result, key, val);
			}

			start = i+1;
		}
	}

	
	return result;
}

bool fredc_validate_json(char* contents, size_t length) {
	if ( (contents == 0) || (length == 0) )  {
		fprintf(stderr, "invalid json: empty\n");
		return false;
	}

	size_t start = 0, end = length;
	while (contents[start] != '{' && start < end) start++;
	while (contents[end] != '}' && end > start) end--;

	if ( start >= end) {
		fprintf(stderr, "invalid json: no top level object\n");
		return false;
	}
	
	return true;
}

void fredc_val_free(fredc_val* v) {
	switch(v->type) {
		JSON_STRING: {
			free(v->string.data);
		} break;
		JSON_OBJ: {
			fredc_obj_free(&v->object);
		} break;
		JSON_LIST: {
			free(v->list.data);
		} break;
		
		default: break;
	}
	v->type = JSON_NULL;
}

void fredc_node_free(fredc_node* n) {
	fredc_val_free(&n->val);
	free(n->key.data);
}

void fredc_obj_free(fredc_obj* o) {
	for (int i = 0; i < o->length; i++) {
		fredc_node* node = o->props+i;
		while(node) {
			fredc_node_free(node);
			node = node->next;
		}
	}
	free(o->props);
	free(o->pool.data);
	o->pool = (fredc_node_list){0};
}

#endif
