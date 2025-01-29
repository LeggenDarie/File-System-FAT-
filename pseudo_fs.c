#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    /*
    printf("DEBUG: FAT state after initialization:\n");
    for (int i = 0; i < NUM_CLUSTERS; i++) {
        printf("FAT[%d] = %d\n", i, fat[i]);  // Should all be -1
    }
    */

    // Initialize file table: set all entries as unused
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].in_use = 0;
    }
    
    // Print entire file table to check if initialization is correct
    /*
    printf("DEBUG: File table state after initialization:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        printf("File %d -> in_use: %d, name: '%s'\n", i, file_table[i].in_use, file_table[i].name);
    }
    */
    printf("File system initialized!\n");
}

// Function to find a free cluster in the FAT
int findFreeCluster() {
    for (int i = 0; i < NUM_CLUSTERS; i++) {
        if (fat[i] == -1) {
            return i;
        }
    }
    return -1;
}

// Function to create a file
void createFile(const char *fileName) {
    // Find a free slot in the file table
    int fileIndex = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("ERROR: No space left in the file table!\n");
        return;
    }

    // Find a free cluster in the FAT
    int freeCluster = findFreeCluster();
    if (freeCluster == -1) {
        printf("ERROR: No free clusters available!\n");
        return;
    }

    // Assign the file details
    strncpy(file_table[fileIndex].name, fileName, 50);
    file_table[fileIndex].start_block = freeCluster;
    file_table[fileIndex].size = 0;
    file_table[fileIndex].in_use = 1;
    file_table[fileIndex].is_dir = 0;

    // Mark the cluster as used in the FAT
    fat[freeCluster] = 0;  // For now, we use 0 to indicate "end of file"

    printf("File '%s' created successfully! (File Index: %d, Start Block: %d)\n", 
           file_table[fileIndex].name, fileIndex, file_table[fileIndex].start_block);
}

// Debug function to print the file table
void printFileTable() {
    printf("\nCurrent File Table:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use) {
            printf("File %d: %s (Start Block: %d, Size: %d bytes)\n", 
                   i, file_table[i].name, file_table[i].start_block, file_table[i].size);
        }
    }
    printf("\n");
}

int main() {
    initFileSystem();

    // Test file creation
    createFile("example.txt");
    createFile("test.doc");

    // Print file table to check if files are added correctly
    printFileTable();
    
    return 0;
}
