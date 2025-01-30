#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// File system parameters
#define FS_SIZE (1024 * 1024)
#define CLUSTER_SIZE 1024
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

// FileHandle structure to track file position
typedef struct {
    int file_index; 
    int position;
} FileHandle;

// Memory-mapped file system
void *fs_memory = NULL;
int *fat;
FileEntry *file_table;

void initFileSystem() {
    // Allocate memory with mmap
    fs_memory = mmap(NULL, FS_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (fs_memory == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    
    printf("mmap() successful! Memory mapped at: %p\n", fs_memory);

    // Assign memory to FAT and file table
    fat = (int *)fs_memory;
    file_table = (FileEntry *)(fat + NUM_CLUSTERS);

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
    int freeCluster = findFreeCluster();
    if (freeCluster == -1) {
        printf("ERROR: No free clusters available!\n");
        return;
    }
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].in_use) {
            strcpy(file_table[i].name, fileName);
            file_table[i].start_block = freeCluster;
            file_table[i].size = 0;
            file_table[i].in_use = 1;
            fat[freeCluster] = 0;
            printf("File '%s' created successfully! (File Index: %d, Start Block: %d)\n", fileName, i, freeCluster);
            return;
        }
    }
    printf("ERROR: Maximum file limit reached!\n");
}

// Function to erase a file
void eraseFile(const char *fileName) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use == 1 && strcmp(file_table[i].name, fileName) == 0) {
            int cluster = file_table[i].start_block;
            while (cluster != -1) {
                int next = fat[cluster];
                fat[cluster] = -1; // Free cluster
                cluster = next;
            }
            file_table[i].in_use = 0;
            printf("File '%s' erased!\n", fileName);
            return;
        }
    }
    printf("ERROR: File '%s' not found!\n", fileName);
}

// Function to open a file and return a FileHandle
FileHandle openFile(const char *fileName) {
    FileHandle fh = {-1, 0};
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use == 1 && strcmp(file_table[i].name, fileName) == 0) {
            fh.file_index = i;
            fh.position = 0;
            printf("File '%s' opened (File Index: %d, Position: %d)\n", fileName, fh.file_index, fh.position);
            return fh;
        }
    }

    printf("ERROR: File '%s' not found!\n", fileName);
    return fh;
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

    // Print the file table before creating a file
    printf("\nFILE TABLE BEFORE CREATING A FILE:\n");
    printFileTable();

    // Print FAT state before creating a file
    printf("\nFAT STATE BEFORE CREATING A FILE:\n");
    for (int i = 0; i < 10; i++) {
        printf("FAT[%d] = %d\n", i, fat[i]);
    }

    // Test file creation
    createFile("example.txt");
    createFile("test.doc");

    // Print the file table after creating a file
    printf("\nFILE TABLE AFTER CREATING A FILE:\n");
    printFileTable();

    // Print FAT state after creating a file
    printf("\nFAT STATE AFTER CREATING A FILE:\n");
    for (int i = 0; i < 10; i++) {
        printf("FAT[%d] = %d\n", i, fat[i]);
    }

    // Open file and check if the FileHandle works
    printf("\nOpening file 'example.txt'...\n");
    FileHandle fh = openFile("example.txt");

    // Print FileHandle details
    printf("\nFILE HANDLE STATE (AFTER OPENING):\n");
    printf("File Index: %d\n", fh.file_index);
    printf("File Position: %d\n", fh.position);

    fh.position = 10;
    printf("Manually moved position to %d\n", fh.position);

    // Print updated FileHandle details
    printf("\nUPDATED FILE HANDLE STATE:\n");
    printf("File Index: %d\n", fh.file_index);
    printf("File Position: %d\n", fh.position);

    // Print the file table again after opening a file
    printf("\nFILE TABLE AFTER OPENING A FILE:\n");
    printFileTable();

    // Test file erase
    eraseFile("example.txt");

    // Print file table to check if file is erased correctly
    printf("\nFILE TABLE AFTER ERASING A FILE:\n");
    printFileTable();

    return 0;
}
