#include <stdio.h>
#include <stdlib.h>

// File system parameters
#define FS_SIZE (1024 * 1024)
#define CLUSTER_SIZE 512 
#define NUM_CLUSTERS (FS_SIZE / CLUSTER_SIZE)

#define MAX_FILES 100

// Structure for a file entry or a directory
typedef struct {
    char name[50];  
    int start_block;  
    int size;  
    int in_use;  
    int is_dir; 
} FileEntry;

// File Allocation Table (FAT)
int fat[NUM_CLUSTERS];

// File table with fixed entries
FileEntry file_table[MAX_FILES];  


void initFileSystem() {
    // Initialize FAT: set all clusters as free (-1)
    for (int i = 0; i < NUM_CLUSTERS; i++) {
        fat[i] = -1;
    }
    
    // Print entire FAT to check if initialization is correct
    printf("DEBUG: FAT state after initialization:\n");
    for (int i = 0; i < NUM_CLUSTERS; i++) {
        printf("FAT[%d] = %d\n", i, fat[i]);  // Should all be -1
    }

    // Initialize file table: set all entries as unused
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].in_use = 0;
    }
    
    // Print entire file table to check if initialization is correct
    printf("DEBUG: File table state after initialization:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        printf("File %d -> in_use: %d, name: '%s'\n", i, file_table[i].in_use, file_table[i].name);
    }

    printf("File system initialized!\n");
}


int main() {
    initFileSystem();
    return 0;
}
