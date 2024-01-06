#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "ds.h"

BootEntry* boot;
char* hex_sha;
char* mmap_info;
int* fat;
int displacement;
int dir_entry_per_cluster;
DirEntry* root;

void printRootDirectory();
void restoreWithSha(char* input_file);
void restoreWithSha1(char* input_file);
void restoreWithoutSha(char* input_file);

void printRemind() {
    printf("Usage: ./nyufile disk <options>\n");
    printf("  -i                     Print the file system information.\n");
    printf("  -l                     List the root directory.\n");
    printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
    printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

void readData(int argc, char** argv) {

    struct stat stats;
    char* fat_name = argv[1]; 

    int fd = open(fat_name, O_RDWR);

    if(fd == -1) {
        printRemind();
        exit(0);
    }

    if (fstat(fd, &stats) == -1) {
        printf("Cannot Get File Statistics\n");
        exit(1);
    }

    mmap_info = mmap(NULL, stats.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_info == MAP_FAILED) {
        printf("Mmap Fails\n");
        exit(1);
    };

    boot = (BootEntry*) mmap_info;
    fat = (int*) (mmap_info+(boot->BPB_RsvdSecCnt * boot->BPB_BytsPerSec));
    int byte_per_sec = boot->BPB_BytsPerSec;
    int fat_sec = boot->BPB_FATSz32 * boot->BPB_NumFATs;
    int root_cluster_dis = ((boot->BPB_RootClus - 2) * boot->BPB_SecPerClus);
    displacement =  (boot->BPB_RsvdSecCnt + fat_sec + root_cluster_dis) * byte_per_sec;
    dir_entry_per_cluster = (int) (boot->BPB_BytsPerSec * boot->BPB_SecPerClus / 32);
    root = (struct DirEntry*) (mmap_info + displacement);
}

int main(int argc, char** argv) {

    if(argc == 1) {
        printRemind();
        exit(0);
    }

    if(argc == 2) {
        printRemind();
        exit(0);
    }

    optind = 2;
    char arg_option = getopt(argc, argv, "ilr:R:s:");

    // print out the boot information
    if(arg_option == 'i') {

        readData(argc, argv);
        printf("Number of FATs = %d\n",  boot->BPB_NumFATs);
        printf("Number of bytes per sector = %d\n",  boot->BPB_BytsPerSec);
        printf("Number of sectors per cluster = %d\n", boot->BPB_SecPerClus);
        printf("Number of reserved sectors = %d\n", boot->BPB_RsvdSecCnt);

    // print out the root directory
    } else if(arg_option == 'l') {

        readData(argc, argv);
        printRootDirectory();

    } else if(arg_option == 'r' || arg_option == 'R') {


        char* input_file = optarg;

        // check if after '-r' there are other opt
        char arg_option_2 = getopt(argc, argv, "ilr:R:s:");

        if(arg_option_2 == 's') {

            // with hash sha
            hex_sha = optarg;

            // call restore with hash
            readData(argc, argv);
            restoreWithSha(input_file);
            exit(0);

        } else if(arg_option_2 == -1) {

            // call restore without hash
            readData(argc, argv);
            restoreWithoutSha(input_file);
            exit(0);
        }

        printRemind();
        exit(0);

    } else {
        printRemind();
        exit(0);
    }

    return 0;
}

