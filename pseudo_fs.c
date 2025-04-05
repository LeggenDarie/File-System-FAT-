#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// File system parameters
#define FS_SIZE (1024 * 1024)
#define CLUSTER_SIZE 1024
#define NUM_CLUSTERS (FS_SIZE / CLUSTER_SIZE)
#define DISK_FILE "disk.bin"
#define MAX_FILES 100
#define DATA_OFFSET (sizeof(int) * NUM_CLUSTERS + sizeof(FileEntry) * MAX_FILES)

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

    int fd = open(DISK_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("open failed");
        exit(1);
    }

    // Make sure the file is exactly FS_SIZE bytes
    if (ftruncate(fd, FS_SIZE) < 0) {
        perror("ftruncate failed");
        close(fd);
        exit(1);
    }

    // Allocate memory with mmap
    fs_memory = mmap(NULL, FS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs_memory == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(1);
    }
    close(fd);
    
    printf("mmap() successful! Memory mapped at: %p (file: %s)\n", fs_memory, DISK_FILE);

    // Assign memory to FAT and file table
    fat = (int *)fs_memory;
    file_table = (FileEntry *)(fat + NUM_CLUSTERS);

    int is_new_disk = 1;
    // Initialize FAT: set all clusters as free (-1)
    for (int i = 0; i < NUM_CLUSTERS; i++) {
        if (file_table[i].in_use) {
            is_new_disk = 0;
            break;
        }
    }

    if (is_new_disk) {
        printf("New disk, initializing FAT and file table...\n");
        for (int i = 0; i < NUM_CLUSTERS; i++) {
            fat[i] = -1;
        }
        for (int i = 0; i < MAX_FILES; i++) {
            file_table[i].in_use = 0;
        }
    } else {
        printf("Existing file system found in disk.bin\n");
    }
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
                fat[cluster] = -1;
                cluster = next;
            }
            file_table[i].in_use = 0;
            printf("File '%s' erased!\n", fileName);
            return;
        }
    }
    printf("ERROR: File '%s' not found!\n", fileName);
}

void writeFile(FileHandle *fh, const void *buffer, int size) {
    if (fh->file_index == -1) {
        printf("Invalid FileHandle!\n");
        return;
    }

    int cluster = file_table[fh->file_index].start_block;
    void *cluster_start = (void *)((char *)fs_memory + DATA_OFFSET + CLUSTER_SIZE * cluster);

    memcpy((char*)cluster_start + fh->position, buffer, size);
    fh->position +=size;
    if (fh->position > file_table[fh->file_index].size) {
        file_table[fh->file_index].size = fh->position;
    }
    printf("Wrote %d bytes to file '%s' (cluster %d)\n", size, file_table[fh->file_index].name, cluster);
}

void readFile(FileHandle *fh, void *buffer, int size) {
    if (fh->file_index == -1) {
        printf("Invalid FileHandle!\n");
        return;
    }

    int cluster = file_table[fh->file_index].start_block;
    void *cluster_start = (void *)((char *)fs_memory + DATA_OFFSET + CLUSTER_SIZE * cluster);

    memcpy(buffer, (char *)cluster_start + fh->position, size);
    fh->position += size;
    printf("Read %d bytes from file '%s' (cluster %d)\n", size, file_table[fh->file_index].name, cluster);
}

// Function to open a file and return a FileHandle
FileHandle openFile(const char *fileName) {
    FileHandle fh = {-1, 0};
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].in_use == 1 && strcmp(file_table[i].name, fileName) == 0) {
            fh.file_index = i;
            fh.position = file_table[i].size;
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
    printf("\n[FAT-FS] File system avviato.\n");
    int running = 1;
    while (running) {
        printf("\nScegli un'operazione:\n");
        printf("1. Mostra FAT\n");
        printf("2. Mostra File Table\n");
        printf("3. Crea un file\n");
        printf("4. Scrivi in un file\n");
        printf("5. Leggi da un file\n");
        printf("6. Cancella un file\n");
        printf("0. Esci\n");
        printf(">> ");

        int choice;
        scanf("%d", &choice);
        getchar();

        char name[50];
        char buffer[1024];
        int size;

        switch (choice) {
            case 1:
                printf("\nFAT STATE:\n");
                for (int i = 0; i < 10; i++) {
                    printf("FAT[%d] = %d\n", i, fat[i]);
                }
                break;
            case 2:
                printFileTable();
                break;
            case 3:
                printf("Inserisci il nome del file da creare: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                createFile(name);
                break;
            case 4:
                printf("Nome file da aprire per scrittura: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                FileHandle fh_w = openFile(name);
                if (fh_w.file_index != -1) {
                    fh_w.position = file_table[fh_w.file_index].size;
                    printf("Inserisci il contenuto da scrivere: ");
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strcspn(buffer, "\n")] = 0;
                    writeFile(&fh_w, buffer, strlen(buffer));
                }
                break;
            case 5:
                printf("Nome file da aprire per lettura: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                FileHandle fh_r = openFile(name);
                if (fh_r.file_index != -1) {
                    fh_r.position = 0;
                    readFile(&fh_r, buffer, 1024);
                    printf("Contenuto letto: %s\n", buffer);
                }
                break;
            case 6:
                printf("Nome del file da cancellare: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                eraseFile(name);
                break;
            case 0:
                printf("Uscita dal file system.\n");
                running = 0;
                break;
            default:
                printf("Scelta non valida.\n");
        }
    }
    return 0;
}
