#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
	char *path = "root/dylans_folder/something.txt";
	printf("path = %s\n", path);
	
	char *part = strtok(path, "/");
	while(part != NULL) {
		printf("part = %s\n", part);
		part = strtok(NULL, "/");
	}
}