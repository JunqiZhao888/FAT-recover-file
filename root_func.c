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

void printName(DirEntry* root, unsigned char* dir_name, int is_dir, int file_size) {

    int start_cluster = root->DIR_FstClusLO | (root->DIR_FstClusHI << 16);

    if(is_dir) {

        for(int j = 0; j < 8; j++) {
            if(dir_name[j] == 0 || dir_name[j] == ' ') break;
            printf("%c", dir_name[j]);
        }
                
        printf("/ (starting cluster = %d)\n", start_cluster);

    } else {

        for(int j = 0; j < 8; j++){
            if(dir_name[j] == 0 || dir_name[j] == ' ') break;
            printf("%c", dir_name[j]);
        }

        if(dir_name[8] != ' ') {
            printf("%c", '.');
            for(int j = 8; j < 11; j++){
                if(dir_name[j] == 0 || dir_name[j] == ' ') break;
                printf("%c", dir_name[j]);   
            }   
        }
        
        if(file_size == 0) {
                printf(" (size = 0)\n");
        } else {
                printf(" (size = %d, starting cluster = %d)\n", file_size, start_cluster);
        }

    }
}

void printRootDirectory() {

    int num_dir_entry = 0;
    int cluster_seq = boot->BPB_RootClus;
    
    while(1) {

        for(int i = 0; i < dir_entry_per_cluster; i++) {

            unsigned char* dir_name = root->DIR_Name;

            if(dir_name[0] == 0) {
                break;
            }
            
            if(dir_name[0] == 0xE5){
                root++;
                continue;
            } 

            num_dir_entry += 1;
            int dir_verify = root->DIR_Attr == 0x10;
            int file_size = root->DIR_FileSize;

            printName(root, dir_name, dir_verify, file_size);

            root++;
        }

        if(fat[cluster_seq] >= 0x0ffffff8){
            printf("Total number of entries = %d\n", num_dir_entry);
            exit(0);
        }
        
        int last_cluster = cluster_seq;
        cluster_seq = fat[cluster_seq];
        int displace_of_cluster = cluster_seq - last_cluster;
        displacement += boot->BPB_BytsPerSec * (displace_of_cluster * boot->BPB_SecPerClus);
        root = (struct DirEntry*) (mmap_info + displacement); 
    }
}