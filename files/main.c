//
//  main.c
//  assignment4_dthiemann
//
//  Created by Dylan Thiemann on 11/15/14.
//  Copyright (c) 2014 Dylan Thiemann. All rights reserved.
//

/*
 
 FAT like file system that contains all the information in one file.
 
 1) MBR
 2) FAT
 3) Data
 
 FAT Entry = 16 bit integer
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "list.h"

/**********************************************/
/* Function Declaration */

int get_current_time();
int get_current_date();
int get_available_cluster_in_bytes(uint16_t fat_len);
entry_t get_entry_from_cluster(uint16_t cluster);

/**********************************************/

/* Global variables */
mbr_t myMBR;
FILE *my_file;
fat_t *fat;
char *file_name = "FAT.bin";

/* Formats the FAT file system */
void format(int16_t sector_size, int16_t cluster_size, uint16_t disk_size) {
    uint16_t cluster_in_bytes = cluster_size * sector_size;
    /* Most difficult to implement */
    my_file = fopen(file_name, "wb+");
    
    myMBR.sector_sz = sector_size;
    myMBR.cluster_sz = cluster_size;
    myMBR.disk_sz = disk_size;
    
    /* Initialize the FAT with -1 for unnalocated */
    fat = malloc(sizeof(fat_t)*myMBR.fat_len);
    for (int i = 0; i < myMBR.fat_len; i++) {
        fat[i].state = 0xFFFF;
    }
    
    for (int i = 0; i < 6; i++) {
        myMBR.disk_name[i] = file_name[i];
    }
    
    myMBR.data_start = (sizeof(myMBR) + sizeof(fat))/cluster_in_bytes;

    
    /** 
     Create root directory 
     */
    
    entry_t *root_dir = malloc(sizeof(entry_t));
    
    root_dir->creation_date = get_current_date();
    root_dir->creation_time = get_current_time();
    
    char *root_name = "root";
    for (int i = 0; i < 4; i++) {
        root_dir->name[i] = root_name[i];
    }
    root_dir->name_len = 4;
    root_dir->entry_type = 1;   /* Directory */
    root_dir->size = 0;         /* 0 b/c directory */
    
    mkdir(root_dir->name, 0700);
    
    /** 
     write data to disk 
     */
    
    fwrite(&myMBR, sizeof(cluster_size*sector_size), 1, my_file);
    
    int fat_size_in_cluster = sizeof(fat)/cluster_in_bytes;
    int fat_size = (fat_size_in_cluster + 1) * cluster_in_bytes;
    
    /* FAT size = disk size divided by cluster size */
    int max_fat_length = disk_size - 1 - (fat_size_in_cluster + 1);
    
    myMBR.fat_start = 1;
    myMBR.fat_len = max_fat_length;
    
    fat[0].state = 0;
    fat[1].state = 1;
    
    fseek(my_file, cluster_in_bytes, 0);
    fwrite(&fat, sizeof(fat_size), 1, my_file);
    
    /* Move to first data segment */
    fseek(my_file, (myMBR.data_start)*cluster_in_bytes, 0);
    fwrite(&root_dir, cluster_in_bytes, 1, my_file);
    
    /**
     Save file
     */
    fclose(my_file);
}

/* Returns the cluster number of where the directory starts */
int fs_opendir(char *absolute_path) {
    struct stat s;
    int err = stat("/path/to/possible_dir", &s);
    
    /* Path exists */
    if (err != -1) {
        int num = 0;
        fat_t *myFAT = getFAT();
        
        /* Iterate through FAT until empty or found */
        uint16_t cluster_num = myFAT[num];
        while (cluster_num != 0xFFFF) {
            entry_t temp_entry = get_entry_from_cluster(num);
            /**
             Need to figure out how to 
             iterate through children
             */
        }
        
        /* No match was found */
        return -1;
    }
    /* Path doesn't exist */
    else {
        printf("Path: %s does not exist", absolute_path);
    }
}

void fs_mkdir(int dh, char *child_name) {
    
}

entry_t *fs_ls(int dh, void *prev) {
    return NULL;
}

/* File operations */
int fs_open(char *absolute_path, char *mode) {
    return 0;
}

int fs_close(int fh) {
    return 0;
}

int fs_write(const void *buffer, int count, int stream) {
    return 0;
}

int fs_read(const void *buffer, int count, int stream) {
    return 0;
}


/**********************************************/


/* Optional implementations */

int fs_rmdir() {
    return 0;
}

int fs_rm() {
    return 0;
}

/**********************************************/

/* Helper Functions */

/* Returns current date in a 16 bit integer */
int get_current_date() {
    time_t t = time(NULL);
    struct tm *tptr = localtime(&t);
    
    int year = tptr->tm_year - 80;  // years since 1980
    int month = tptr->tm_mon;  // months since january (0-11)
    int day = tptr->tm_mday; // day of the month (1-31)
    
    uint16_t result = (year << 9) | (month << 5) | (day << 0);
    return result;
}

/* Returns current time in a 16 bit integer */
int get_current_time() {
    time_t t = time(NULL);
    struct tm *tptr = localtime(&t);
    
    int hour = tptr->tm_hour; // hour of the day (0-23)
    int min = tptr->tm_min;  // minutes after the hour (0-60)
    int sec = tptr->tm_sec; // seconds in the minute (0-61) or (0-60) after c99
    sec = sec / 2;
    
    uint16_t result = (hour << 11) | (min << 5) | (sec << 0);
    
    return result;
}

/* Finds the next available cluster to write data to */
int get_available_cluster_in_bytes(uint16_t fat_len) {
    for (int i = 0; i < fat_len; i++) {
        if (fat[i].state == 0xFFFF) {
            return i;
        }
    }
    return fat_len;
}

/* get FAT from disk */
fat_t * getFAT() {
    FILE *my_file = fopen(file_name, "rb+");
    mbr_t *myMBR;
    
    fread(&myMBR, sizeof(mbr_t), 1, my_file);
    
    fseek(my_file, myMBR->cluster_sz * myMBR->sector_sz, 0);
    fat_t *myFat;
    
    fread(&myFat, myMBR->fat_len * myMBR->cluster_sz * myMBR->sector_sz, 1, my_file);
    
    return myFat;
}

/* get MBR from disk */
mbr_t * getMBR() {
    FILE *my_file = fopen(file_name, "rb+");
    mbr_t *myMBR;
    
    fread(&myMBR, sizeof(mbr_t), 1, my_file);
    
    return myMBR;
}

/* Takes poitner to a data slot and converts it to bytes */
uint16_t clusterToBytes(mbr_t *myMBR, uint16_t address) {
    uint16_t data_start_bytes = myMBR->data_start * myMBR->cluster_sz * myMBR->sector_sz;
    return data_start_bytes + address * myMBR->cluster_sz * myMBR->sector_sz;
}

/* Get an entry from a cluster number */
entry_t get_entry_from_cluster(uint16_t cluster) {
    FILE *f = fopen(file_name, "r");
    mbr_t *myMBR = getMBR();
    fat_t *myFAT = getFAT();
    
    /* Nothing exists at that cluster */
    if (myFAT[cluster] == 0xFFFF) { return NULL; }
    
    /* Find where the data starts */
    uint16_t cluster_size_in_bytes = myMBR.cluster_size * myMBR.sector_sz;
    uint16_t data_start_in_bytes = myMBR.data_start * cluster_size_in_bytes;
    uint16_t cluster_start = data_start_in_bytes + cluster_size_in_bytes * cluster;
    
    entry_t my_entry;
    fseek(f, cluster_start, 0);
    fread(my_entry, sizeof(entry_t), 1, f);
    
    return my_entry;
    
}

/* Prints contents of disk */
void print_disk() {
    FILE *f;
    //mbr_t * mbr_test = malloc(sizeof(mbr_t));
    mbr_t *mbr_test = getMBR();
    //fat_t fat_entry;
    entry_t entry;
    entry_ptr_t ptr;
    int i, j;
    f = fopen(file_name, "rb");
    fread(mbr_test, sizeof(mbr_t), 1, f);
    printf("============  %s  ============\n", mbr_test->disk_name);
    printf("SECTOR SIZE:  %d bytes\n", mbr_test->sector_sz);
    printf("CLUSTER SIZE: %d sector(s)\n", mbr_test->cluster_sz);
    printf("DISK SIZE:    %d cluster(s)\n", mbr_test->disk_sz);
    printf("FAT START:    %dst cluster\n", mbr_test->fat_start);
    printf("FAT LENGTH:   %d clusters\n", mbr_test->fat_len);
    printf("DATA START:   %dnd cluster\n", mbr_test->data_start);
    
    fat_t * entries = getFAT();
    printf("============  FAT INFO  ============\n");
    fseek(f, (mbr_test->sector_sz * mbr_test->cluster_sz) - 1, SEEK_SET);
    /* Part G */
    
    int numRequiredFatEntries = myMBR.fat_len;
    
    for(i = 0; i < numRequiredFatEntries; i++)
        printf("%d %d \n", entries[i].state, i);
    
    printf("============  DATA INFO  ===========\n");
    for (i = 0; i < numRequiredFatEntries; i++) {
        if (entries[i].state !=  0xFFFF) {
            fseek(f, clusterToBytes(mbr_test, entries[i].state), SEEK_SET);
            fread(&entry, sizeof(entry_t), 1, f);
            if (entry.entry_type != 2){
                printf("ENTRY:    %s\n", entry.name);
                //printCreationDate(entry.creation_date);
                //printCreationTime(entry.creation_time);
                printf("NUMBER OF CHILDREN:      %d\n", entry.numChildren);
                for (j = 0; j < entry.numChildren && entry.numChildren > 0; j++){
                    fread(&ptr, sizeof(entry_ptr_t), 1, f);
                    printf("-- POINTER\n");
                    printf("---- TYPE:        %d\n", ptr.type);
                    printf("---- RESERVED:    %d\n", ptr.reserved);
                    printf("---- START:       %d\n", ptr.start);
                    if (ptr.type == 2)
                    {
                        j = j - 1;
                        printf("-- LINKED CLUSTER DETECTED; MOVING TO CLUSTER %d\n", ptr.start);
                        fseek(f, clusterToBytes(mbr_test, ptr.start), SEEK_SET);
                        fread(&entry, sizeof(entry_t), 1, f);
                    }
                }
            }
        }
    }
    free(mbr_test);
    free(entries);
    fclose(f);
}









int main(int argc, const char * argv[]) {
    // insert code here...
    //printf("%d\n", 3/2);
    
    format(64, 1, 2000);
    
    printf("%s\n", myMBR.disk_name);
    
    return 0;
}
