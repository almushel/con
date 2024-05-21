#ifndef FREDC_H
#define FREDC_H

#include <stdbool.h>
#include <stddef.h>

#include "darr.h"
#include "str8.h"

enum fredc_data_types {
	JSON_UNDEFINED = 0,
	JSON_NULL,
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
bool fredc_validate_json(const char* contents, size_t length);
fredc_obj fredc_parse_obj_str(const char* contents, size_t length);
fredc_list fredc_parse_list_str(const char* contents, size_t length);

fredc_val fredc_get_prop(fredc_obj* obj, const char* key);
fredc_val fredc_set_prop(fredc_obj* obj, const char* key, fredc_val val);

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

#define FREDC_OBJ_MIN 2

static size_t fredc_hash(fredc_obj* obj, str8 key) {
	size_t result = 0;

	for (int i = 0; i < key.length; i++) {
		result += key.data[i] << 1;
	}

	return result % obj->length;
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

void fredc_push_prop(fredc_obj* obj, str8 key, fredc_val prop) {
	size_t index = fredc_hash(obj, key);
	assert(index < obj->length);

	str8 key2;
	key2.length = key.length;
	key2.data = (char*)malloc(key2.length+1);
	memcpy(key2.data, key.data, key2.length);

	fredc_node* dest = obj->props + index;
	if (dest->key.length == 0) {
		dest->key = key2;
		dest->val = prop;
	} else if (str8_cmp(dest->key, key2) ) {
		free(key2.data);
		fredc_val_free(&dest->val);
		dest->val = prop;
	} else {
		while (dest->next) {
			dest = dest->next;
		}

		fredc_node node = {.key = key2, .val = prop};
		darr_push(obj->pool, node);
		dest->next = obj->pool.data + (obj->pool.length-1);
	}
}

fredc_node* fredc_get_node(fredc_obj* obj, str8 key) {
	if (key.data == 0 || key.length == 0) { return 0; }
	size_t index = fredc_hash(obj, key);

	fredc_node* node = obj->props+index;
	while (node && !str8_cmp(node->key, key)) { 
		node = node->next;
	}

	if (node && (node->key.length == 0 || node->key.data == 0)) {
		node = 0;
	}

	return node;
}

// obj: target object
// key: string describing the location of the property in dot notation (e.g. [obj.]"foo.bar") 
// returns: val on success or undefined fredc_val on failure
fredc_val fredc_get_prop(fredc_obj* obj, const char* key) {
	fredc_val result = {};

	str8_list keys = str8_split(new_str8(key, strlen(key), true), '.', true);
	fredc_node* node = fredc_get_node(obj, keys.data[0]);
	if (node == 0) { goto cleanup; }

	if (keys.length > 1) {
		if (node->val.type == JSON_OBJ) {
			result = fredc_get_prop(&node->val.object, keys.data[1].data);
		} else {
			result = (fredc_val){};
		}
	} else {
		result = node->val;
	}

	cleanup:
		free(keys.data);
		return result;
}

// obj: target object
// key: string describing the location of the property in dot notation (e.g. [obj.]"foo.bar") 
// val: the fred_c val to be set
// returns: val on success or undefined fredc_val on failure
//
// Will replace only the last key and fail on any preceding key that is not an object.
fredc_val fredc_set_prop(fredc_obj* obj, const char* key, fredc_val val) {
	fredc_val result = {};

	str8_list keys = str8_split(new_str8(key, strlen(key), true), '.', true);
	fredc_node* node = fredc_get_node(obj, keys.data[0]);

	if (keys.length > 1) {
		if (node == 0) {
			fredc_obj new_obj = new_fredc_obj(0);
			result = fredc_set_prop(&new_obj, keys.data[1].data, val);
			
			if (result.type == val.type) {
				fredc_val obj_val = {
					.type = JSON_OBJ,
					.object = new_obj
				};
				fredc_push_prop(obj, keys.data[0], obj_val);

			} else {
				fredc_obj_free(&new_obj);
			}
		} else if (node->val.type == JSON_OBJ){ 
			result = fredc_set_prop(&node->val.object, keys.data[1].data, val);
		} else {
			printf("can't set properties of non-object %s\n", node->key.data);
			result = (fredc_val){};
		}
	} else {
		fredc_push_prop(obj, keys.data[0], val);
		result = val;
	}

	free(keys.data);

	return result;
}

#define INDENT_SIZE 4

str8 fredc_val_str8ify(fredc_val val, int indent) {
	str8 result = {};

	switch (val.type) {
		case JSON_NULL: {
			result = new_str8("null", 4, true);
		} break;

		case JSON_BOOL: {
			result = new_str8(val.boolean ? "true" : "false", 4 + (size_t)(!val.boolean), true);
		} break;

		case JSON_STRING: {
			str8 quote = new_str8("\"", 1, true);
			str8 list[] = {quote, val.string, quote};
			result = str8_list_concat((str8_list) { .data = list, .length = 3 });
		} break;

		case JSON_NUM: {
			int len = snprintf(0,0, "%f", val.number);
			if (len) {
				result = new_str8("", len, false);
				snprintf(result.data, result.length+1, "%f", val.number);
			}
		} break;

		case JSON_OBJ: {
			int count = 0;
			str8_list str_list = {};
			darr_push(str_list, new_str8("{\n", 2, true));

			for (int i = 0; i < val.object.length; i++) {
				fredc_node* node = val.object.props+i;
				while (node) {
					if (node->key.length) {
						darr_push(str_list, fredc_node_str8ify(*node, indent+1)); 
						darr_push(str_list, new_str8(",\n", 2, true));
						count++;
					}
					node = node->next;
				}
			}

			if (count) {
				str_list.length--; // Remove trailing comma
				darr_push(str_list, new_str8("\n", 1, true));

				str8 indent_str = new_str8("", (indent*INDENT_SIZE), false);
				memset(indent_str.data, ' ', indent_str.length);
				darr_push(str_list, indent_str);
				darr_push(str_list, new_str8("}", 1, true));
				
				result = str8_list_concat(str_list);
				// free indent_str
			} else {
				result = new_str8("{}", 2, true);
			}

			free(str_list.data);
		} break;

		case JSON_LIST: {
			if (val.list.length) {
				str8_list str_list = {};
				darr_push(str_list, new_str8("[\n", 2, true));

				str8 indent_str = new_str8("", (indent+1)*INDENT_SIZE, false);
				memset(indent_str.data, ' ', indent_str.length);

				for (int i = 0; i < val.list.length; i++) {
					darr_push(str_list, indent_str);
					darr_push(str_list, fredc_val_str8ify(val.list.data[i], indent+1));
					
					if (i != (val.list.length)-1) {
						darr_push(str_list, new_str8(",\n", 2, true));
					} else {
						darr_push(str_list, new_str8("\n", 1, true));
					}
				}

				darr_push(str_list, new_str8(indent_str.data, indent_str.length-INDENT_SIZE, true));
				darr_push(str_list, new_str8("]", 1, true));
				
				result = str8_list_concat(str_list);
				free(str_list.data);
				//free indent_str
			} else {
				result = new_str8("[]", 2, true);
			}
		} break;

		default: {
			result = new_str8("undefined", 9, true);
		} break;
	}

	return result;
}

str8 fredc_node_str8ify(fredc_node prop, int indent) {
	str8 indent_str = new_str8("", indent*INDENT_SIZE, false);
	memset(indent_str.data, ' ', indent_str.length);

	str8 str_list[] = {
		indent_str,
		new_str8("\"", 1, true),
		prop.key,
		new_str8("\": ", 3, true),
		fredc_val_str8ify(prop.val, indent)
	};

	// TODO: Figure out why indent_str becomes garbage data for single property object

	str8 result = str8_list_concat(
		(str8_list) {
			.data = str_list,
			.length = 5,
			.capacity = 5
		}
	);

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

fredc_val fredc_parse_val(const char* contents, size_t length) {
	fredc_val result = {};
	double num_val = 0;

	str8 valstr = new_str8(contents, length, false);
	
	if (contents[0] == '\"' && contents[length-1] == '\"') {
		str8 val = str8_trim(
			new_str8((char*)contents, length, true),
			new_str8("\"", 1, true),
			true
		);

		result.type = JSON_STRING;
		result.string.length = val.length;
		result.string.data = (char*)malloc(val.length+1);
		memcpy(result.string.data, val.data, val.length);
		result.string.data[val.length] = '\0';
	} else if ( str8_cmp(valstr, new_str8("true", 4, true)) ){
		result.type = JSON_BOOL;
		result.boolean = true;
	} else if ( str8_cmp(valstr, new_str8("false", 5, true)) ){
		result.type = JSON_BOOL;
		result.boolean = false;
	} else if ( str8_cmp(valstr, new_str8("null", 4, true)) ) {
		result.type = JSON_NULL;
	} else if ( (num_val = strtod(contents, 0)) ){
		result.type = JSON_NUM;
		result.number = num_val;
	}

	return result;
}

fredc_val_list fredc_parse_list_str(const char* contents, size_t length) {
	fredc_list result = {};	

	if ((contents[0] == '[') && (contents[length-1] == ']')) {
		str8 cs = str8_trim(
			new_str8((char*)contents, length, true), 
			new_str8("[]", 2, true),
			true
		);
		str8_list val_strs = str8_split(cs, ',', true);
		
		for (int i = 0; i < val_strs.length; i++) {
			if (val_strs.data[i].length) {
				val_strs.data[i] = str8_trim_space(val_strs.data[i], true);	
				fredc_val item = fredc_parse_val(val_strs.data[i].data, val_strs.data[i].length);
				darr_push(result, item);
			}
		}
	}

	return result;
}

fredc_obj fredc_parse_obj_str(const char* contents, size_t length) {
	str8 cs = str8_trim_space(new_str8(contents, length, true), true);
	if (cs.length == 0 || cs.data[0] != '{' || cs.data[cs.length-1] != '}') {
		return (fredc_obj){};
	}

	cs = str8_trim(cs, new_str8("{}", 2, true), true);
	fredc_obj result = new_fredc_obj(0);

	int start = 0;
	str8* vals;
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
				
			vals = str8_cut(new_str8(cs.data+start, end-start+1, true), ':', true);
			if (vals) {
				str8 key = str8_trim(
					str8_trim_space(vals[0], true),
					new_str8("\"", 1, true),
					true
				);
				str8 objstr = str8_trim_space(vals[1], true);

				fredc_val new_obj = {
					.type = JSON_OBJ,
					.object = fredc_parse_obj_str(objstr.data, objstr.length),
				};
				fredc_push_prop(&result, key, new_obj);

				free(vals);
			}
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
			
			vals = str8_cut(new_str8(cs.data+start, end-start+1, true), ':', true);
			if (vals) {
				str8 key = str8_trim(
					str8_trim_space(vals[0], true),
					new_str8("\"", 1, true),
					true
				);
				str8 liststr = str8_trim_space(vals[1], true);

				fredc_val new_list = {
					.type = JSON_LIST,
					.list = fredc_parse_list_str(liststr.data, liststr.length)
				};
				fredc_push_prop(&result, key, new_list);
				free(vals);
			}
			
			i = end+1;
			start = i+1;
		} else if (cs.data[i] == ',' || i == cs.length-1) {
			vals = str8_cut(new_str8(cs.data+start, i-start+1, true), ':', true);
			if (vals) {
				if (vals[0].length && vals[1].length) {
					str8 key = str8_trim(
						str8_trim_space(vals[0], true),
						new_str8("\",", 2, true),
						true
					);
					str8 valstr = str8_trim(
						str8_trim_space(vals[1], true),
						new_str8(",", 1, true),
						true
					);
					fredc_val val = fredc_parse_val(valstr.data, valstr.length);

					fredc_push_prop(&result, key, val); 
				}
				free(vals);
			}

			start = i+1;
		}
	}

	
	return result;
}

bool fredc_validate_json(const char* contents, size_t length) {
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
	v->type = JSON_UNDEFINED;
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
