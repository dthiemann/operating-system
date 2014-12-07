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
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "list.h"

/**********************************************/
/* Helper Function Declaration */

int get_current_time();
int get_current_date();
int get_available_cluster_in_bytes(uint16_t fat_len);
int get_available_cluster(uint16_t fat_len);
entry_t get_entry_from_cluster(uint16_t cluster);
int find_open_child_slot_for_cluster(int directory_handler);
uint16_t clusterToBytes(mbr_t *myMBR, uint16_t address);
entry_ptr_t get_children_data_from_cluster(uint16_t cluster, int num_child);

mbr_t * getMBR();
uint16_t * getFAT();
void write_full(uint8_t *ptr, size_t bytes, FILE *f);
void write_full_entry(entry_t *ptr, size_t bytes, FILE *f);
void write_full_mbr(mbr_t *ptr, size_t bytes, FILE *f);
void save_fat();

void read_full_entry(entry_t *ptr, size_t bytes, FILE *f);
entry_t get_root_directory();
char *get_absolute_path_from_handler(int dh);

int get_number_of_files();
void analyze_new_directory(char *current_path, entry_t *my_entry, int cluster);
void create_file_or_directory(char *path, entry_t *my_entry);

/**********************************************/

/* Global variables */
mbr_t *myMBR;
FILE *my_file;
uint16_t *fat;
char *file_name;
slist_t *open_files;

int cluster_sz_bytes;
uint16_t data_start_in_bytes;

ssize_t fat_size;
uint16_t alloc_entries;
uint16_t total_entries;

#define	FAT_FREE		0xFFFF
#define FAT_END			0XFFFE
#define	FAT_UNALLOC		0xFFFD


void format(uint16_t sector_sz, uint16_t cluster_sz, uint16_t disk_sz) {
    //file_name = "disk";
    cluster_sz_bytes = cluster_sz * sector_sz;
    /* Most difficult to implement */
    my_file = fopen(file_name, "rb+");
    
    /* Settng up FAT */
    uint8_t *extended_mbr = (uint8_t *)malloc(cluster_sz_bytes);
    bzero(extended_mbr, cluster_sz_bytes); /* fill with zeros */
    
    myMBR = (mbr_t*)(&extended_mbr[0]);
    
    myMBR->sector_sz = sector_sz;
    myMBR->cluster_sz = cluster_sz;
    myMBR->disk_sz = disk_sz;
    myMBR->fat_start = 1;
    
    double fat_length = disk_sz * sizeof(uint16_t) / ((double) cluster_sz * sector_sz);
    fat_length = ceil(fat_length);
    
    myMBR->fat_len = fat_length;
    myMBR->data_start = fat_length + 1;
    
    data_start_in_bytes = myMBR->data_start * cluster_sz_bytes;
    
    strcpy(myMBR->disk_name, "disk");
    printf("fat start = %d len= %d \n", myMBR->fat_start, myMBR->fat_len);
    
    //save the mbr to disk
    //write_full((uint8_t *) &extended_mbr[0], cluster_sz_bytes, my_file);
    write_full_mbr(myMBR, sizeof(mbr_t), my_file);
    //fwrite(&myMBR, sizeof(cluster_sz_bytes), 1, my_file);
    
    // allocate the fat
    fat = (uint16_t *) malloc(myMBR->fat_len * cluster_sz_bytes);
    alloc_entries = disk_sz;
    total_entries = myMBR->fat_len * cluster_sz_bytes / sizeof(uint16_t);
    
    printf("fat alloc = %d total = %d\n", alloc_entries, total_entries);
    
    for (int i = 0; i < alloc_entries; i++) {
        fat[i] = FAT_FREE;
    }
    for (int i = alloc_entries + 1; i < total_entries; i++) {
        fat[i] = FAT_UNALLOC;
    }
    
    fat_size = myMBR->fat_len * cluster_sz_bytes;
    
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
    /* Updated FAT to point to cluster */
    fat[0] = 0;
    save_fat();
    
    /* Write root directory to FAT.bin */
    
    fseek(my_file, (myMBR->data_start)*cluster_sz_bytes, 0);
    printf("where to write %d\n", (myMBR->data_start)*cluster_sz_bytes);
    write_full_entry(root_dir, sizeof(entry_t), my_file);
    //fwrite(&root_dir, cluster_sz_bytes, 1, my_file);
    
    free(extended_mbr);
    
    open_files = init();
    
    list_item_t *rd = (list_item_t *)malloc(sizeof(list_item_t));
    rd->value = 0;
    rd->isDirectory = 1;
    rd->the_file = NULL;
    strcpy(rd->mode, "\0");
    strcpy(rd->path, "root/");
    
    add(open_files, rd);
    
    fclose(my_file);
}
/* Returns the cluster number of where the directory starts */
int fs_opendir(char *absolute_path) {
    my_file = fopen(file_name, "rb+");
    
    struct stat s;
    int err = stat(absolute_path, &s);
    
    /* Path exists */
    if (err != -1) {
        //char *part = (char *) malloc(sizeof(char) * strlen(absolute_path));
        int path_index = 0;
        
        char part_array[strlen(absolute_path)];
        strcpy(part_array, absolute_path);
        char *part = strtok(part_array, "/");
        /* How many directories will we traverse */
        while (part != NULL) {
            part = strtok(NULL, "/");
            path_index = path_index + 1;
        }
        
        
        int num = 0;        /* FAT navigation */
        int count = 0;      /* for path */
        
        //my_file = fopen(file_name, "rb+");

        uint16_t *myFAT = fat;
        
        part = strtok(part_array, "/");
        /* Iterate through FAT until empty or found */
        uint16_t cluster_num = myFAT[num];
        
        /* count represents the portion of the path we are at */
        while (count < path_index) {
            
            entry_t temp_entry = get_entry_from_cluster(cluster_num);
            
            /* Find directory */
            if (strcmp(part, temp_entry.name) == 0) {
                int child_num = 0;
                /* Read next part of path */
                part = strtok(NULL, "/");
                /* Found the child */
                if (part == NULL) { return cluster_num; }
                
                /* Search the children */
                while (child_num < temp_entry.numChildren) {
                    entry_ptr_t child = get_children_data_from_cluster(cluster_num, child_num);
                    entry_t temp_temp_entry = get_entry_from_cluster(child.start);
                    
                    /* Found a matching child */
                    if (strcmp(temp_temp_entry.name, part) == 0) {
                        count = count + 1;
                        cluster_num = child.start;      /* Use new cluster */
                        break;
                    }
                    
                    child_num++;
                }
                /* Update count variable for path directories */
                count++;
            }
            else {
                cluster_num++;
                
                /* Return -1 when we reach end of file */
                if (myFAT[cluster_num] == 0xFFFF || myFAT[cluster_num] == 0xFFFD) {
                    fclose(my_file);
                    return -1;
                }
            }
        }
        list_item_t *new_item = (list_item_t *)malloc(sizeof(list_item_t));
        new_item->value = cluster_num;
        strcpy(new_item->mode, "\0"); /* Directory */
        new_item->the_file = NULL; /* Directory */
        new_item->isDirectory = 1;
        strcpy(new_item->path, absolute_path);
        
        free(part);
        fclose(my_file);
        return cluster_num;
    }
    /* Path doesn't exist */
    else {
        printf("Path: %s does not exist", absolute_path);
    }
    fclose(my_file);
    return -1;
}

void fs_mkdir(int dh, char *child_name) {
    my_file = fopen(file_name, "rb+");
    
    mbr_t *mbr = myMBR;

    //FILE *my_file = fopen(file_name, "wb+");
    entry_t parent_dir = get_entry_from_cluster(dh);
    int child_location = find_open_child_slot_for_cluster(dh);

    int child_cluster = get_available_cluster(mbr->fat_len);
    int child_cluster_local = (child_cluster + myMBR->data_start) * cluster_sz_bytes;
    
    /* Updated number of children for parent directory */
    entry_t parent_entry = get_entry_from_cluster(dh);
    parent_entry.numChildren = parent_entry.numChildren + 1;
    fseek(my_file, clusterToBytes(mbr, dh), SEEK_SET);
    /* Write updated parent dir to file */
    fwrite(&parent_entry, sizeof(entry_t), 1, my_file);
    
    /* Create the new directory */
    entry_t new_dir;
    strncpy(new_dir.name, child_name, sizeof(new_dir.name));
    new_dir.name[sizeof(new_dir.name) - 1] = '\0';
    
    new_dir.entry_type = 1;     /* Directory */
    new_dir.creation_date = get_current_date();
    new_dir.creation_time = get_current_time();
    new_dir.size = 0;
    new_dir.numChildren = 0;
    
    fseek(my_file, child_cluster_local, SEEK_SET);
    fwrite(&new_dir, sizeof(entry_t), 1, my_file);
    
    /* Root dir currently only takes up 1 cluser */
    fat[child_cluster] = FAT_END;
    
    /* Create a pointer */
    entry_ptr_t new_pointer;
    new_pointer.type = 1;
    new_pointer.start = child_cluster;
    
    fseek(my_file, child_location, SEEK_SET);
    fwrite(&new_pointer, sizeof(entry_ptr_t), 1, my_file);
    
    
    char *path = get_absolute_path_from_handler(dh);
    //printf("path: %s \n", temp);
    char total_path[strlen(path) + strlen(child_name) + 2];
    strcpy(total_path, path);
    strcat(total_path, child_name);
    mkdir(total_path, 0700);
    
    save_fat();
    
    fclose(my_file);
}

/**
 prev - represents the # of child to get
 */
entry_t *fs_ls(int dh, int child_num) {
    my_file = fopen(file_name, "rb+");
    
    entry_ptr_t child_data = get_children_data_from_cluster(dh, child_num);
    if (&child_data == NULL) { return NULL; }
    
    
    entry_t entry = get_entry_from_cluster(child_data.start);
    entry_t *entry_2 = (entry_t *)malloc(sizeof(entry_t));
    
    /* copy data */
    entry_2->entry_type = entry.entry_type;
    entry_2->creation_time = entry.creation_time;
    entry_2->creation_date = entry.creation_date;
    entry_2->name_len = entry.name_len;
    strcpy(entry_2->name, entry.name);
    entry_2->size = entry.size;
    entry_2->numChildren = entry.numChildren;
    
    //printf("name of child from within fs_ls %s\n", entry_2->name);
    
    fclose(my_file);
    
    return entry_2;
}

/* File operations */
int fs_open(char *absolute_path, char *mode) {
    
    /* Display error if file mode is incorrect */
    if (strcmp(mode, "w") != 0) {
        perror("Must open file in write mode");
        return -1;
    }
    
    int is_present = 0;
    
    /* Check if the file exists */
    if (access( absolute_path, F_OK ) != -1) { is_present = 1; }
    
    /* Find number of subdirectories */
    char absolute_path_array[strlen(absolute_path)];
    strcpy(absolute_path_array, absolute_path);
    char *part_of_path = strtok(absolute_path_array, "/");
    
    // Get number of directories used in the path
    int number_of_dirs = 0;
    while (part_of_path != NULL) {
        number_of_dirs++;
        part_of_path = strtok(NULL, "/");
    }
    
    /* Check if parent path exists */
    char *parent_path = (char *) malloc (sizeof(absolute_path));
    strcpy(absolute_path_array, absolute_path);
    
    number_of_dirs--;
    
    char *temp;     /* used for inidividual directory names */
    /* Get the parent path */
    parent_path = strtok(absolute_path_array, "/");
    strcat(parent_path, "/");

    for (int i = 0; i < number_of_dirs-1; i++) {
        
        temp = strtok(NULL, "/");
        printf("temp = %s\n", temp);
        strcat(parent_path, temp);
        strcat(parent_path, "/");
    }
    /* Parent path doesn't exist, return error */
    if (access( parent_path, F_OK ) == -1) {
        printf("broke access \n");
        return -1;
    }
    /* Add file to a cluster and write to disk if it doesn't exist */
    uint16_t cluster;
    if (is_present == 0) {
       
        printf("We are here 7\n");
        /* root directory */
        strcpy(absolute_path_array, absolute_path);
        
        part_of_path = strtok(absolute_path_array, "/");
        printf("We are here 7\n");
        
        
        myMBR = getMBR();
        fat = getFAT();
        
        /* root entry */
        int cluster_num = 0;
        entry_t entry = get_entry_from_cluster(cluster_num);
        printf("We are here 8\n");
        
        /* Get parent cluster */
        int current_dir_num = 0;
        
        /** 
         ERROR BELOW HERE SOMEWHERE
         */
        
        while (current_dir_num < number_of_dirs) {
            
            part_of_path = strtok(NULL, "/");
            printf("We are here 7\n");
            int number_of_children = entry.numChildren;
            int child_num = 0;
            
            /* Iterate through all the children to find appropriate directory */
            while(child_num < number_of_children) {
                entry_ptr_t child_ptr = get_children_data_from_cluster(cluster_num, child_num);
                entry_t child = get_entry_from_cluster(child_ptr.start);
                
                /* if child is next in path traverse through this directory */
                if (strcmp(part_of_path, child.name) == 0 && child.entry_type == 1) {
                    cluster_num = child_ptr.start;
                    entry = child;
                    break;
                }
                child_num++;
            }
            current_dir_num++;
        }
        
        /* Have cluster of parent directory, need to add child */
        cluster = get_available_cluster(myMBR->fat_len);
        uint16_t cluster_local_bytes = get_available_cluster_in_bytes(myMBR->fat_len);
        
        entry_t *new_entry = (entry_t *) malloc(sizeof(entry_t));
        
        /* Create new entry */
        char *f_name;
        int i = 0;
        while (i < number_of_dirs + 1) {
            f_name = strtok(absolute_path, "/");
        }
        strcpy(new_entry->name,f_name);
        new_entry->name_len = sizeof(f_name);
        new_entry->entry_type = 0;
        new_entry->creation_time = get_current_time();
        new_entry->creation_date = get_current_date();
        new_entry->size = 0;
        new_entry->numChildren = 0;
        
        entry_ptr_t *child_entry = (entry_ptr_t *)malloc(sizeof(entry_ptr_t));
        child_entry->type = 0;
        child_entry->start = cluster;
        
        /* Write child to bin file */
        fseek(my_file, cluster_local_bytes, SEEK_SET);
        fwrite(&new_entry, sizeof(cluster_sz_bytes), 1, my_file);
        
        int child_local = find_open_child_slot_for_cluster(cluster_num);
        fseek(my_file, child_local, SEEK_SET);
        //write_full(&child_entry)
        fwrite(&child_entry, sizeof(entry_ptr_t), 1, my_file);
        
    }
    
    /* Parent path exists... create file */
    FILE *my_file = fopen(absolute_path, mode);
    
    /* Need to work on this */
    list_item_t new_open_file;
    new_open_file.value = cluster;
    new_open_file.the_file = my_file;
    strcpy(new_open_file.mode, mode);
    strcpy(new_open_file.path, absolute_path);
    new_open_file.isDirectory = 0;
    
    /* Add file to open files data structures */
    add(open_files, &new_open_file);

    return new_open_file.value;
}

int fs_close(int fh) {
    list_item_t *my_item = get_list_item_with_handler(open_files, fh);
    
    fclose(my_item->the_file);
    return 0;
}

/* Loads a file system given under disk_file */
void load_disk(char *disk_file) {
    char *path = "/root";
    
    file_name = disk_file;
    mbr_t *mbr = getMBR();
    uint16_t *fat = getFAT();
    
    /* Read root directory */
    uint16_t root_cluster = mbr->data_start;
    entry_t current = get_entry_from_cluster(root_cluster);
    
    int entrys = 0;
    int num_entries = get_number_of_files();
    for (int i = 0; i < current.numChildren; i++ ) {
        entry_ptr_t child_ptr = get_children_data_from_cluster(root_cluster, i);
        entry_t child = get_entry_from_cluster(child_ptr.start);
        if (child.entry_type == 1) {
            analyze_new_directory(path, &child, child_ptr.start);
        }
        
        /* Write the file */
        create_file_or_directory(path, &child);
    }
}


/**
 Writes an item to a file
 
 Returns 1 for success and -1 for failure
 */

int fs_write(const void *buffer, int count, int stream) {
    
    list_item_t *my_item = get_list_item_with_handler(open_files, stream);
    
    // File is in the wrong mode
    if (strcmp(my_item->mode, "r") == 0) { return -1; }
    
    FILE *my_file = my_item->the_file;
    fwrite(buffer, count, 1, my_file);
    
    // Success
    return 1;
}

/**
 Reads an item from a file
 
 Returns 1 for sucess and -1 for failure
 */

int fs_read(const void *buffer, int count, int stream) {
    
    list_item_t *my_item = get_list_item_with_handler(open_files, stream);
    
    
    if (strcmp(my_item->mode, "w") == 0) { return -1; }
    
    FILE *my_file = my_item->the_file;
    fread(&buffer, count, 1, my_file);
    
    return 1;
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
/**********************************************/
/**********************************************/

/* Helper Functions */

/**********************************************/
/**********************************************/
/**********************************************/

/* Creates teh physical directory/file in the root directory */
void create_file_or_directory(char *path, entry_t *my_entry) {
    char *full_path = (char *) malloc(sizeof(path) + sizeof(my_entry->name));
    strcat(full_path, path);
    strcat(full_path, "/");
    strcat(full_path, my_entry->name);
    
    /* Check type */
    if (my_entry->entry_type == 0) {
        FILE *f = fopen(full_path, "w+");
        fclose(f);
    } else if (my_entry->entry_type == 1) {
        mkdir(full_path, 0700);
    }
}

/* Gets the absolute path of the parent directory from the handler */
char *get_absolute_path_from_handler(int dh) {
    list_item_t *file_node = get_list_item_with_handler(open_files, dh);
    return file_node->path;
}

/* goes through all the children of a directory */
void analyze_new_directory(char *current_path, entry_t *my_entry, int cluster) {
    char *new_path = (char *)malloc(sizeof(current_path) + sizeof(my_entry->name));
    strcat(new_path, current_path);
    strcat(new_path, "/");
    strcat(new_path, my_entry->name);
    
    for(int i = 0; i < my_entry->numChildren; i++) {
        entry_ptr_t child_ptr = get_children_data_from_cluster(cluster, i);
        entry_t child = get_entry_from_cluster(child_ptr.start);
        if (child.entry_type == 1) {
            analyze_new_directory(new_path, &child, child_ptr.start);
        }
        
        /* Write the file */
        create_file_or_directory(new_path, &child);
    }
}

/* Returns a handling to the root directory */
entry_t get_root_directory() {
    entry_t root_dir = get_entry_from_cluster(0);
    return root_dir;
}

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
int get_available_cluster(uint16_t fat_len) {
    for (int i = 0; i < fat_len; i++) {
        if (fat[i] == 0xFFFF) {
            return i;
        }
    }
    /* if disk is full */
    return fat_len;
}

/* Finds the next available cluster to write data to */
int get_available_cluster_in_bytes(uint16_t fat_len) {
    mbr_t *mbr = myMBR;
    uint16_t clust_sz_bytes = mbr->cluster_sz * mbr->sector_sz;
    
    for (int i = 0; i < fat_len; i++) {
        if (fat[i] == 0xFFFF) {
            return i * clust_sz_bytes;
        }
    }
    /* if disk is full */
    return fat_len * clust_sz_bytes;
}

/* get FAT from disk */
uint16_t * getFAT() {
    
    //fseek(my_file, 0, 0);
    fread(myMBR, sizeof(mbr_t), 1, my_file);
    
    long num = myMBR->cluster_sz * myMBR->sector_sz;
    
    fseek(my_file, num, SEEK_SET);
    uint16_t *myFat;

    fread(&myFat, myMBR->fat_len * myMBR->cluster_sz * myMBR->sector_sz, 1, my_file);
    //fclose(my_file);
    return fat;
}

/* get MBR from disk */
mbr_t * getMBR() {
    fread(&myMBR, sizeof(mbr_t), 1, my_file);
    
    //fclose(my_file);
    return myMBR;
}

/* Takes poitner to a data slot and converts it to bytes */
uint16_t clusterToBytes(mbr_t *myMBR, uint16_t address) {
    uint16_t data_start_bytes = myMBR->data_start * myMBR->cluster_sz * myMBR->sector_sz;
    return data_start_bytes + address * myMBR->cluster_sz * myMBR->sector_sz;
}

/* Get an entry from a cluster number */
entry_t get_entry_from_cluster(uint16_t cluster) {
    //FILE *f = my_file;
    uint16_t *myFAT = fat;
    

    entry_t my_entry;
    /* Nothing exists at that cluster */
    if (myFAT[cluster] == 0xFFFF) {
        return my_entry;
    }
    

    /* Find where the data starts */
    uint16_t cluster_start = data_start_in_bytes + cluster_sz_bytes * cluster;

    //printf("where to read %d %ld\n", cluster_start, ftell(my_file));
    
    fseek(my_file, cluster_start, SEEK_SET);
    int value = fread(&my_entry, 1, sizeof(entry_t), my_file);
    //printf("value = %d\n", value);
    //read_full_entry(&my_entry, sizeof(entry_t), f);
    
    return my_entry;
    
}

/* Get a specific child of a directory */
entry_ptr_t get_children_data_from_cluster(uint16_t cluster, int num_child) {
    FILE *f = my_file;
    uint16_t *myFAT = fat;
    
    entry_ptr_t my_child;
    
    /* Nothing exists at that cluster */
    if (myFAT[cluster] == 0xFFFF) { return my_child; }
    
    /* Find where the data starts */
    uint16_t cluster_size_in_bytes = myMBR->cluster_sz * myMBR->sector_sz;
    uint16_t data_start_in_bytes = myMBR->data_start * cluster_size_in_bytes;
    uint16_t cluster_start = data_start_in_bytes + cluster_size_in_bytes * cluster;
    
    fseek(f, cluster_start + sizeof(entry_t) + sizeof(entry_ptr_t)*num_child, 0);
    fread(&my_child, sizeof(entry_ptr_t), 1, f);
    
    return my_child;
    
}

/* Returns the bytes address of a free slot in memory for a new child */
int find_open_child_slot_for_cluster(int directory_handler) {
    mbr_t *mbr = myMBR;
    uint16_t cluster_size_in_bytes = mbr->cluster_sz * mbr->sector_sz;
    
    /* parent directory */
    entry_t parent_dir = get_entry_from_cluster(directory_handler);
    int number_of_children = parent_dir.numChildren;
    
    /* Dir location in bytes */
    uint16_t dir_local = clusterToBytes(mbr, directory_handler);
    
    /* move to first child slot */
    //fseek(my_file, dir_local + sizeof(entry_t), SEEK_SET);
    //uint16_t current_seek = sizeof(entry_t) ;
    //int child_count = 0;
    
    /*
    while (current_seek < cluster_size_in_bytes) {
        entry_ptr_t temp = get_children_data_from_cluster(directory_handler, child_count);
        
        if (&temp == NULL) {
            return dir_local + current_seek;
        } else {
            current_seek = current_seek + sizeof(entry_ptr_t);
            fseek(my_file, dir_local + current_seek, SEEK_SET);
        }
    } */
    return dir_local + sizeof(entry_t) + sizeof(entry_ptr_t)*number_of_children;
}

/**
 Provided by Octav Chipara :-)
 */
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

/* Writes an entry_t */
void write_full_entry(entry_t *ptr, size_t bytes, FILE *f) {
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

/* reads an entry_t */
void read_full_entry(entry_t *ptr, size_t bytes, FILE *f) {
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

void write_full_mbr(mbr_t *ptr, size_t bytes, FILE *f) {
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

/**
 Provided by Octav Chipara :-)
 */
void save_fat() {
    fseek(my_file, cluster_sz_bytes + 1, SEEK_SET);
    write_full((uint8_t *)(&fat[0]), fat_size,  my_file);
}

int get_number_of_files() {
    mbr_t *mbr = getMBR();
    uint16_t *fat = getFAT();
    int count = 0;
    
    for(int i = 0; i < mbr->fat_len; i++) {
        if (fat[i] != 0xFFFF && fat[i] != 0xFFFD) {
            count = count + 1;
        }
    }
    return count;
}

/**********************************************/
/**********************************************/
/**********************************************/

/* Prints contents of disk */
void print_disk() {
    FILE *f;
    //mbr_t * mbr_test = malloc(sizeof(mbr_t));
    mbr_t *mbr_test = getMBR();
    //fat_entry;
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
    
    uint16_t * entries = getFAT();
    printf("============  FAT INFO  ============\n");
    fseek(f, (mbr_test->sector_sz * mbr_test->cluster_sz) - 1, SEEK_SET);
    /* Part G */
    
    int numRequiredFatEntries = myMBR->fat_len;
    
    for(i = 0; i < numRequiredFatEntries; i++)
        printf("%d %d \n", entries[i], i);
    
    printf("============  DATA INFO  ===========\n");
    for (i = 0; i < numRequiredFatEntries; i++) {
        if (entries[i] !=  0xFFFF) {
            fseek(f, clusterToBytes(mbr_test, entries[i]), SEEK_SET);
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
    
    /* Create the disk file */
    file_name = "disk";
    my_file = fopen(file_name, "wb+");
    fclose(my_file);
    
    format(64, 1, 2048);
    int dh = fs_opendir("root/");
    //printf("directory handler = %d\n", dh);
    
    fs_mkdir(dh, "dylans_folder");
    fs_mkdir(dh, "dylans_folder2");
    fs_mkdir(dh, "dylans3");
    
    /*
    int num_children = 0;
    while (num_children < 3) {
        entry_t *child = fs_ls(0, num_children);
        printf("child name = %s\n", child->name);
        num_children++;
        //free(child);
    }*/
    int result = fs_open("root/my_file.txt", "w");
    
    return 0;
}
