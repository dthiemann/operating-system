// version 1.1

/**
 sector_size (2 bytes) - the size of a sector in bytes (must be at least 64 bytes)
 cluster_size (2 bytes) - the size of the cluster in sectors (must be at least 1)
 disk_size (2 bytes) - the size of the disk in clusters
 fat_start, fat_length (2 bytes) - the start/length of the FAT area on the disk
 data_start, data_length (2 bytes) - the start/length of the Data area on the disk
 disk_name - the name of the disk. The name is guaranteed not to exceed 31 bytes and should be stored as a null-terminated string
 **/

typedef struct __attribute__ ((__packed__)) {
    uint16_t sector_sz;
    uint16_t cluster_sz;
    uint16_t disk_sz;
    uint16_t fat_start, fat_len;
    uint16_t data_start;
    char	 disk_name[32];
} mbr_t;

/**
 entry_type (1 byte) - indicates if this is a file/directory (0 - file, 1 - directory)
 creation_time (2 bytes) - format described below
 creation_date (2 bytes) - format described below
 length of entry name (1 byte)
 entry name (16 bytes) - the file/directory name
 size (4 bytes) - the size of the file in bytes. Should be zero for directories:
 **/
typedef struct __attribute__ ((__packed__)) {
    uint8_t		entry_type;
    uint16_t	creation_time, creation_date;
    uint8_t		name_len;
    char		name[16];
    uint32_t	size;
    uint16_t    numChildren;
} entry_t;


/**
 pointer type (1 byte) - (0 = pointer to a file, 1 = pointer to a directory, 2 = pointer to another entry describing more children for this directory)
 reserved (1 byte)
 start_pointer (2 bytes) - points to the start of the entry describing the child
 **/
typedef struct __attribute__ ((__packed__)) {
    uint8_t	type;
    uint8_t reserved;
    uint16_t start;
} entry_ptr_t;

/**
 FAT entry
 */
typedef struct __attribute__ ((__packed__)) {
    uint16_t state;
} fat_t;

// -- added support for initializing the list

typedef struct list_item {
    int value;
    FILE *the_file;
	struct list_item *next;
	struct list_item *prev;
} list_item_t; 

/**
global variable declaration
**/
typedef struct single_list {
	list_item_t *head;
	list_item_t *tail;
} slist_t;

void init(slist_t *list);
void add(slist_t *list, char *value);
void print(slist_t *list);
void empty(slist_t *list);
void bublesort(slist_t *list);

int get_num_elements(slist_t *list);