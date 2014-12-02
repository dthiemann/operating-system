#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

typedef struct __attribute__ ((__packed__)) {
	uint16_t sector_sz;
	uint16_t cluster_sz;
	uint16_t disk_sz;
	uint16_t fat_start, fat_len;
	uint16_t data_start;
	char	 disk_name[32];
} mbr_t;

int cluster_sz_bytes;

mbr_t *mbr;										// mbr
uint8_t *extended_mbr = NULL;					// the full cluster of mbr

#define	FAT_FREE		0xFFFF
#define FAT_END			0XFFFE
#define	FAT_UNALLOC		0xFFFD

uint16_t *fat = NULL;							// the fat extended to cluster
uint16_t alloc_entries;							// 
uint16_t total_entries;
ssize_t fat_size;								// fat size in bytes

FILE* file;

void write_full(uint8_t *ptr, size_t bytes, FILE *f) {
	do {
		int sz = fwrite(ptr, 1, bytes, f);
		if (sz < 0) {
			perror("write error");
			exit(0);
		}
		bytes = bytes - sz;
		ptr = &ptr[sz];
	} while (bytes > 0);
}

void save_fat() {
	fseek(file, cluster_sz_bytes * 1, SEEK_SET);
	write_full((uint8_t *)(&fat[0]), fat_size, file);
}

void format(uint16_t sector_sz, uint16_t cluster_sz, uint16_t disk_sz) {
	cluster_sz_bytes = cluster_sz * sector_sz;
	
	extended_mbr = (uint8_t *)malloc(cluster_sz_bytes);
	bzero(extended_mbr, cluster_sz_bytes);
	mbr = (mbr_t *) (&extended_mbr[0]);

	mbr->sector_sz = sector_sz;
	mbr->cluster_sz = cluster_sz;
	mbr->disk_sz = disk_sz;
	mbr->fat_start = 1;
	
	double fat_length = disk_sz * sizeof(uint16_t) / ((double) cluster_sz * sector_sz);
	fat_length = ceil(fat_length);
	
	mbr->fat_len = fat_length;
	mbr->data_start = fat_length + 1;
	strcpy(mbr->disk_name, "no name");
	
	printf("fat start=%d len=%d\n", mbr->fat_start, mbr->fat_len);	
	
	//save the mbr to disk
	write_full((uint8_t *) &extended_mbr[0], cluster_sz_bytes, file);	
	
	// allocate the fat
	fat = (uint16_t *) malloc(mbr->fat_len * cluster_sz_bytes);
	alloc_entries = disk_sz;
	total_entries = mbr->fat_len * cluster_sz_bytes / sizeof(uint16_t);
	printf("fat alloc=%d total=%d\n", alloc_entries, total_entries);
	for (int i = 0; i < alloc_entries; i++) {
		fat[i] = FAT_FREE;
	}
	for (int i = alloc_entries + 1; i < total_entries; i++) {
		fat[i] = FAT_UNALLOC;
	}
	fat_size = mbr->fat_len * cluster_sz_bytes;
	// fat_size = total_entries * sizeof(uint16_t);
	save_fat();	
}

int main() {
	file = fopen("disk", "wb");
	if (file == NULL) {
		perror("Cannot create disk file");
		return -1;
	}

	format(512, 1, 2048);
	
	fclose(file);
	free(extended_mbr);

	return 0;
}