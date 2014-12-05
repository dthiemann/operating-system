#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	FILE *f = fopen("test", "rb+");
	/*
	fseek(f, 0, 0);
	printf("%lu ftell \n", ftell(f));
	
	char *part = "bogus";
	char *part2 = "Hello from me";
	fwrite(part2, strlen(part2), 1, f);
	fseek(f, 0, 0);
	fwrite(part, strlen(part), 1, f);
	
	char *new_string = (char *) malloc(strlen(part2));
	fseek(f,0,0);
	fread(new_string, strlen(part2), 1, f);
	printf("%s \n", new_string);
	*/
	//fclose(f);
	if (f != NULL) { printf("open\n"); }
	if (f == NULL) {printf("closed\n"); }
}