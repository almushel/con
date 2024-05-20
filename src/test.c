#include "str8.h"
#include <stdio.h>

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

int main(void) {
	fredc_obj obj;
	size_t len;

	for (int i = 0; i < arr_len(json_strs); i++) {
		printf("Parse and stringify %i:\n", i+1);
		len = strlen(json_strs[i]);
		if (fredc_validate_json(json_strs[i], len)){
			obj = fredc_parse_obj_str(json_strs[i], len);

			printf("%s\n", fredc_obj_stringify(obj));

			fredc_obj_free(&obj);
		} else {
			printf("Validation failed:\n%s\n", json_strs[i]);
		}
	}

	printf("str8 pool size: %lu\n", pool.length);
	str8_free_pool();

	return 0;
}
