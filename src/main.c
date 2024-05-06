#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct str8 {
	char* data;
	size_t length;
} str8;

typedef struct str8_node {
	str8 key, val;

	struct str8_node* next;
} str8_node;

typedef struct str8_map {
	str8_node* list;
	size_t length;
} str8_map;

size_t str8_hash(str8 key) {
	size_t val = 0;

	for (int i = 0; i < key.length; i++) {
		val += key.data[i] << 1;
	}

	return val;
}

#define STR8_MAP_MIN 8

str8_map new_str8_map(size_t length) {
	if (length < STR8_MAP_MIN) {
		length = STR8_MAP_MIN;
	}

	str8_map result = (str8_map) {
		.list = calloc(length, sizeof(str8_node)),
		.length = length,
	};

	return result;
}

void push_str8_map(str8_map* map, str8 key, str8 val) {
	size_t index = str8_hash(key) % map->length;
	assert(index < map->length);

	str8_node* dest = map->list + index;
	if (dest->key.length == 0 || strcmp(dest->key.data, key.data) == 0) {
		dest->key = key;
		dest->val = val;
	} else {
		while (dest->next) {
			dest = dest->next;
		}
		dest->next = malloc(sizeof(str8_node));
		*(dest->next) = (str8_node){
			.key = key,
			.val = val,
		};
	}
}

str8 read_all(char* filename) {
	char buf[1024];
	str8 result = (str8){
		.data = malloc(sizeof(buf)),
		.length = sizeof(buf),
	};
	int used = 0;

	FILE* fstream = fopen(filename, "r");

	size_t bytes_read = 0;
	while ((bytes_read = fread(buf, 1, sizeof(buf), fstream)) > 0){
		if (used + bytes_read > result.length) {
			char* new_result = realloc(result.data, result.length * 2);
			if (new_result) {
				result.data = new_result;
				result.length *= 2;
			} else {
				fprintf(stderr, "failed to realloc result\n");
				return result;
			}
		}

		memcpy(result.data+used, buf, bytes_read);
		used += bytes_read;
	}

	if (!feof(fstream) && ferror(fstream)) {
		perror("(read_all) fread");
		free(result.data);

		result.data = 0;
		result.length = 0;

		return result;
	}

	if (used != result.length) {
		char* new_result = realloc(result.data, used);
		if (new_result) {
			result.data = new_result;
			result.length = used;
		}
	}

	fclose(fstream);

	return result;
}

bool validate_json(char* contents, size_t length) {
	if (	(contents == 0) || (length == 0) )  {
		fprintf(stderr, "invalid json: empty file\n");
		return false;
	}

	// TODO: Verify that characters being skipped are whitespace
	size_t start = 0, end = length-1;
	while (contents[start] != '{' && start < end) start++;
	while (contents[end] != '}' && end > start) end--;

	if ( start >= end) {
		fprintf(stderr, "invalid json: no top level object\n");
		return false;
	}
	
	return true;
}

str8_map parse_json_object(char* contents, size_t length) {
	size_t colons = 0;
	for (int c = 0; c < length; c++) {
		if (contents[c] == ',') colons++;	
	}

	str8_map result = new_str8_map(colons);

	str8 key = {}, val = {};
	for (int c = 0; c < length; c++) {
		if (contents[c] == '"') { 
			int start = c+1;
			int i = start;
			while (i < length) {
				if (contents[i] == '"') { 
					contents[i] = '\0';
					break;
				}
				i++;
			}

			if (key.length == 0) {
				key = (str8) {
					.data = contents+start,
					.length = i-start,
				};
			} else {
				val = (str8) {
					.data = contents+start,
					.length = i-start,
				};
			}

			if (key.length && val.length) {
				push_str8_map(&result, key, val);
				key = (str8){};
				val = (str8){};
			}

			c = i;
		}
	}
	
	return result;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: con [filename]\n");		
		return 1;
	}

	str8 contents = read_all(argv[1]);
	if (!validate_json(contents.data, contents.length)) {
		fprintf(stderr, "%s\n", contents.data);
		return 1;
	}

	size_t start = 0, end = contents.length-1;
	while (contents.data[start] != '{' && start < end) start++;
	while (contents.data[end] != '}' && end > start) end--;

	str8_map result = {};
	if (start < end) {
		result = parse_json_object(contents.data+start, contents.length+(end-start));
	}

	printf("{\n");
	const char* indent = "    ";
	for (int i = 0; i < result.length; i++) {
		str8_node* node = result.list + i;
		while (node && node->key.length) {
			printf("%s%s: %s\n", indent, node->key.data, node->val.data);
			node = node->next;
		}
	}
	printf("}\n");

	return 0;
}
