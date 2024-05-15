#include <stdbool.h>
#include <stdio.h>

#include "darr.h"
#define STR8_IMPLEMENTATION
#define FREDC_IMPLEMENTATOIN
#include "fredc.h"

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

	printf("%s\n",
		fredc_node_to_str8(
			(fredc_node) {
				.key = (str8){.data = argv[1], .length = strlen(argv[1])},
				.val.type = JSON_OBJ,
				.val.object = fredc_parse_obj_str(contents.data, contents.length),
		},0).data
	);

	return 0;
}
