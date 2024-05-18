#include <stdbool.h>
#include <stdio.h>

#include "darr.h"
#define STR8_IMPLEMENTATION
#define FREDC_IMPLEMENTATOIN
#include "fredc.h"

darr_def(char);

str8 read_all(char* filename) {
	char buf[1024];
	char_list result = {};

	FILE* fstream = fopen(filename, "r");
	if (!fstream) {
		perror("(read_all) fread");
		goto cleanup;
	}

	size_t bytes_read = 0;
	while ((bytes_read = fread(buf, 1, sizeof(buf), fstream)) > 0){
		darr_push_arr(result, buf, bytes_read);
	}

	if (feof(fstream) || !ferror(fstream)) {
		darr_resize(result, sizeof(result.data[0]), result.length);
	} else {
		free(result.data);
		result = (char_list){};
	}
	fclose(fstream);

	cleanup:
		return (str8) {
			.data = result.data,
			.length = result.length-1
		};
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: con [filename]\n");		
		return 1;
	}

	str8 contents = str8_trim_space(read_all(argv[1]), true);
	if (!fredc_validate_json(contents.data, contents.length)) {
		fprintf(stderr, "%s\n", contents.data);
		return 1;
	}
	str8_free_pool();

	fredc_obj obj = fredc_parse_obj_str(contents.data, contents.length);
	free(contents.data);
	const char* prop_key = "foo.bar.foo.bar";
	fredc_set_prop(&obj, prop_key, (fredc_val){.type = JSON_BOOL, .boolean = true});

	printf("%s\n", fredc_obj_stringify(obj));
	fredc_obj_free(&obj);

	return 0;
}
