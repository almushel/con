#include <stdbool.h>
#include <stdio.h>

#define FREDC_IMPLEMENTATION
#include "fredc.h"

str8 read_all(char* filename) {
	char buf[1024];
	struct char_list {
		char* data;
		size_t length, capacity;
	} result = {};

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
		result = (struct char_list){};
	}
	fclose(fstream);

	cleanup:
		return (str8){
			.data = result.data,
			.length = result.length > 0 ? result.length-1 : 0,
		};
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: fredc [filename]\n");		
		return 1;
	}
	str8 c = read_all(argv[1]);
	if (c.length == 0) {
		return 1;
	}

	if (!fredc_validate_json(c.data, c.length)) {
		fprintf(stderr, "%s\n", c.data);
		return 1;
	}

	fredc_obj obj = fredc_parse_obj_str(c.data, c.length);
	free(c.data);

	printf("%s\n", fredc_obj_stringify(obj));

	str8_free_pool();
	fredc_obj_free(&obj);

	return 0;
}
