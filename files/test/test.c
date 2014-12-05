#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct temp {
	uint16_t yup;
	uint16_t yup2;
} temp_t;

/* reads an entry_t */
void read_full_entry(temp_t *ptr, size_t bytes, FILE *f) {
    do {
        int sz = fread(ptr, 1, bytes, f);
        if (sz < 0) {
            perror("write error");
            exit(0);
        }
        bytes = bytes - sz;
        ptr = &ptr[sz];
    } while (bytes > 0);
}

int main() {
	FILE *f = fopen("test", "rb+");

	fseek(f, 10, 0);
	//printf("%lu ftell \n", ftell(f));
	
	temp_t new_value;
	new_value.yup = 4;
	new_value.yup2 = 5;
	
	printf("new_value %d %d \n", new_value.yup, new_value.yup2);
	
	fwrite(&new_value, sizeof(temp_t), 1, f);
	//fseek(f, 0, 0);
	//fwrite(part, strlen(part), 1, f);
	
	temp_t new_temp2;
	fseek(f,10,0);
	read_full_entry(&new_temp2, sizeof(temp_t), f);
	//fread(&new_temp2, sizeof(temp_t), 1, f);
	
	printf("new_value %d %d \n", new_temp2.yup, new_temp2.yup2);


	if (f != NULL) { printf("open\n"); }
	if (f == NULL) {printf("closed\n"); }
}
