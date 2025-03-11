#ifndef DEFS_H
#define DEFS_H

#include <sys/types.h>
#include <time.h>

#define MAX_FILENAME 100
#define MAX_FILE_SIZE 1024
#define MAX_DIR_SIZE 1024
#define NUM_BLOCKS_MAX 100


typedef enum file_type {
    FILE_TYPE_FILE = 0,
    FILE_TYPE_DIR = 1
} file_type_t;
typedef struct block {
    file_type_t type;
    mode_t mode;
    int status;
    char content[MAX_FILE_SIZE];
    size_t size;
    time_t last_access;
    time_t last_modification;
    time_t creation_time;
    char path[MAX_FILENAME];
    char dir_path[MAX_FILENAME];
    uid_t uid;
    gid_t gid;
} block_t;
struct file_system {
    block_t blocks[NUM_BLOCKS_MAX];
};





#endif