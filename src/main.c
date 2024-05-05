#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_all(char* filename, int* size) {
	char buf[1024];
	char* result = malloc(sizeof(buf));
	int used = 0;
	*size = sizeof(result);

	FILE* fstream = fopen(filename, "r");

	size_t bytes_read = 0;
	while ((bytes_read = fread(buf, 1, sizeof(buf), fstream)) > 0){
		if (used + bytes_read > *size) {
			char* new_result = realloc(result, *size * 2);
			if (new_result) {
				result = new_result;
				*size *= 2;
			} else {
				fprintf(stderr, "failed to realloc result\n");
				return result;
			}
		}

		memcpy(result+used, buf, bytes_read);
		used += bytes_read;
	}

	if (!feof(fstream) && ferror(fstream)) {
		perror("(read_all) fread");
		free(result);
		return 0;
	}

	if (used != *size) {
		char* new_result = realloc(result, used);
		if (new_result) {
			result = new_result;
			*size = used;
		}
	}

	return result;
}

bool validate_json(char* contents, int size) {
	// TODO: Allow for trailing whitespace
	if (	(contents == 0) 	||
		(size == 0) 		||
		(contents[0] != '{') 	||
		(contents[size-1] != '}')) {

		return false;
	}

	
	return true;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: con [filename]\n");		
		return 1;
	}

	int contents_size = 0;
	char* contents = read_all(argv[1], &contents_size);
	if (contents == 0) {
		return 1;
	}

	if (!validate_json(contents, contents_size)) {
		fprintf(stderr, "invalid json file: %s\n", argv[1]);
	}

	return 0;
}
