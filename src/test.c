#include <stdio.h>
#include <string.h>

#define STR8_IMPLEMENTATION
#define FREDC_IMPLEMENTATOIN
#include "fredc.h"

#define arr_len(arr) sizeof(arr) / sizeof(arr[0])

const char* json_strs[] = {
"\n{}\n",

"{\"key\": \"value\"}\n",

"{\n"
"	\"key1\": true,\n"
"	\"key2\": false,\n"
"	\"key3\": null,\n"
"	\"key4\": \"value\",\n"
"	\"key5\": 101\n"
"}\n",

"{\n"
"	\"key\": \"value\",\n"
"	\"key-n\": 101,\n"
"	\"key-o\": {\n"
"		\"inner key\": \"inner value\",\n"
"		\"inner key-o\": {\n"
"			\"inner inner key\": \"inner inner value\"\n"
"		}\n"
"	},\n"
"	\"key-l\": [\"value1\", \"value2\", \"value3\"]\n"
"}\n",

};

bool validate_fredc_obj(fredc_obj* obj, const char* key) {
	str8 key_list[] = {
		new_str8(key, strlen(key), true),
		new_str8(".", 1, true),
		(str8){}
	};
	bool result = true;
	for (int e = 0; e < obj->length; e++) {
		fredc_node* node = obj->props + e;
		while(node) {
			if (node->key.length == 0){
				node = node->next;
				continue;
			}

			key_list[2] = node->key;
			if (node->val.type == JSON_OBJ) {
				str8 label = str8_list_concat(
					(str8_list) {
						.data = key_list,
						.capacity = 3,
						.length = 3,
					}
				);

				result = result && validate_fredc_obj(&node->val.object, label.data);
			} else if (node->val.type == JSON_UNDEFINED) {
				str8 label = str8_list_concat(
					(str8_list) {
						.data = key_list,
						.capacity = 3,
						.length = 3,
					}
				);
				fprintf(stderr, "%s undefined\n", label.data);
				result = false;
			}

			node = node->next;
		}
	}

	return result;
}

int main(void) {
	size_t num_objects = arr_len(json_strs);
	fredc_obj* objects = calloc(num_objects, sizeof(fredc_obj));
	size_t len;
	for (int i = 0; i < num_objects; i++) {
		printf("Parse and stringify %i:\n", i+1);
		len = strlen(json_strs[i]);
		if (fredc_validate_json(json_strs[i], len)){
			objects[i] = fredc_parse_obj_str(json_strs[i], len);

			printf("%s\n", fredc_obj_stringify(objects[i]));
		} else {
			printf("Validation failed for \nobj_%i\n", i+1);
		}
	}


	char label[64];
	for (int i = 0; i < num_objects; i++) {
		snprintf(label, arr_len(label), "obj_%i", i+1);
		if (!validate_fredc_obj(objects + i, label)) {
			fprintf(stderr, "obj_%i validation FAIL\n", i+1);
		}
		fredc_obj_free(objects +i);
	}

	printf("str8 pool size: %lu\n", pool.length);
	str8_free_pool();

	return 0;
}
