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

unsigned char bin_sha[20] = {0};

int compareName(int input_len, unsigned char* dir_name, char* input_name) {

    char input[15] = {0};
    strcpy(input, input_name + 1);

    char dir[15] = {0};
    int dir_res_p = 0;
    int dir_name_p = 1;

    while(dir_name_p < 11) {
        if(dir_name[dir_name_p] != ' ') {
            if(dir_name_p == 8) {
                dir[dir_res_p++] = '.';
            }
            dir[dir_res_p++] = dir_name[dir_name_p++];
        } else {
            if(dir_name_p <= 7) {
                if(dir_name[8] == ' ') break;
                else dir_name_p = 8;
            } else {
                break;
            }
        }
    }

    return !strcmp(input, dir);
}

void restoreContentByHead(DirEntry* tar, char* input_file) {
    int clusters = (tar->DIR_FileSize + boot->BPB_SecPerClus * boot->BPB_BytsPerSec - 1) / (boot->BPB_SecPerClus * boot->BPB_BytsPerSec);
    int head = (tar->DIR_FstClusHI << 16) | tar->DIR_FstClusLO;

    int walk = head;
    for(; walk < head + clusters - 1; walk++) {
        fat[walk] = walk + 1;
    }

    fat[walk] = 0x0ffffff8;
    tar->DIR_Name[0] = input_file[0];
}

void restoreWithSha1(char* input_file) {

    for (int i = 0; i < 20; i++) {
        sscanf(hex_sha + (i * 2), "%2hhx", &bin_sha[i]);
    }

    int cluster_seq = boot->BPB_RootClus;

    while(1) {

        for(int i = 0; i < dir_entry_per_cluster; i++) {

            unsigned char* dir_name = root->DIR_Name;

            if(dir_name[0] == 0 || dir_name[0] == ' ') {
                // this cluster is empty because the first dir does not have a name
                break;
            } else if(dir_name[0] != 0xE5){
                // skip not deleted one
                root++;
                continue;
            } 

            int bytes_per_sec = boot->BPB_BytsPerSec;
            int sec_of_FATs = boot->BPB_NumFATs*boot->BPB_FATSz32;
            int cur_cluster = (((root->DIR_FstClusHI << 16) | root->DIR_FstClusLO)-2);
            int file_content = bytes_per_sec*(boot->BPB_RsvdSecCnt + sec_of_FATs + (cur_cluster * boot->BPB_SecPerClus));
            unsigned char * file_start = (unsigned char *)(mmap_info + file_content);
            unsigned char tmp_sha[20] = {0};
            SHA1(file_start, root->DIR_FileSize, tmp_sha);

            int flag = 1;
            for(int i = 0; i < 20; i++) {
                if(bin_sha[i] != tmp_sha[i]) {
                    flag = 0;
                    break;
                }
            }

            if(flag){
                restoreContentByHead(root, input_file);
                printf("%s: successfully recovered with SHA-1\n", input_file);
                exit(0);
            } 

            root++;
        }

        if(fat[cluster_seq] >= 0x0ffffff8){
            // all the cluster has ended
            printf("%s: file not found\n", input_file); 
            exit(0);
        }

        int last_cluster = cluster_seq;
        cluster_seq = fat[cluster_seq];
        int displace_of_cluster = cluster_seq - last_cluster;
        displacement += boot->BPB_BytsPerSec * (displace_of_cluster * boot->BPB_SecPerClus); 
        root = (struct DirEntry*) (mmap_info + displacement);
    }
}

void restoreWithSha(char* input_file) {

    for (int i = 0; i < 20; i++) {
        sscanf(hex_sha + (i * 2), "%2hhx", &bin_sha[i]);
    }

    int cluster_seq = boot->BPB_RootClus;

    while(1) {

        for(int i = 0; i < dir_entry_per_cluster; i++) {
            unsigned char* dir_name = root->DIR_Name;
            if(dir_name[0] == ' ') {
                // this cluster is empty because the first dir does not have a name
                break;
            }
            
            if(dir_name[0] != 0xE5){
                // skip not deleted one
                root++;
                continue;
            } 

            int len_of_file = strlen(input_file);

            int is_same = compareName(len_of_file, dir_name, input_file);

            if(is_same) {
                int bytes_per_sec = boot->BPB_BytsPerSec;
                int sec_of_FATs = boot->BPB_NumFATs*boot->BPB_FATSz32;
                int cur_cluster = (((root->DIR_FstClusHI << 16) | root->DIR_FstClusLO)-2);
                int file_content = bytes_per_sec*(boot->BPB_RsvdSecCnt + sec_of_FATs + (cur_cluster * boot->BPB_SecPerClus));
                unsigned char * file_start = (unsigned char*) (mmap_info + file_content);
                unsigned char tmp_sha[20] = {0};
                SHA1(file_start, root->DIR_FileSize, tmp_sha);

                int flag = 1;
                for(int i = 0; i < 20; i++) {
                    if(bin_sha[i] != tmp_sha[i]) {
                        flag = 0;
                        break;
                    }
                }

                if(flag){
                    restoreContentByHead(root, input_file);
                    printf("%s: successfully recovered with SHA-1\n", input_file);
                    exit(0);
                }
            }
            
            root++;
        }

        if(fat[cluster_seq] >= 0x0ffffff8){
            // all the cluster has ended
            printf("%s: file not found\n", input_file); 
            exit(0);
        }

        int last_cluster = cluster_seq;
        cluster_seq = fat[cluster_seq];
        int displace_of_cluster = cluster_seq - last_cluster;
        displacement += boot->BPB_BytsPerSec * (displace_of_cluster * boot->BPB_SecPerClus);
        root = (struct DirEntry*) (mmap_info + displacement);
    }
}

void restoreWithoutSha(char* input_file) {

    int cluster_seq = boot->BPB_RootClus;

    int count = 0;
    DirEntry* candidate = NULL;

    while(1) {

        for(int i = 0; i < dir_entry_per_cluster; i++) {

            unsigned char* dir_name = root->DIR_Name;

            if(dir_name[0] == 0 || dir_name[0] == ' ') {
                // this cluster is empty because the first dir does not have a name
                break;
            } 
            
            if(dir_name[0] != 0xE5){
                // skip not deleted one
                root++;
                continue;
            } 

            int len_of_file = strlen(input_file);
            
            int is_same = compareName(len_of_file, dir_name, input_file);

            if(is_same) {
                count++;
                if(count == 1) candidate = root;
                else if(count > 1) {
                    printf("%s: multiple candidates found\n", input_file);
                    exit(0);
                } 
            } 

            root++;
        }

        if(fat[cluster_seq] >= 0x0ffffff8){
            // all the cluster has ended
            if(candidate != NULL) {
                restoreContentByHead(candidate, input_file);
                printf("%s: successfully recovered\n", input_file);
            } else printf("%s: file not found\n", input_file); 

            exit(0);
        }

        int last_cluster = cluster_seq;
        cluster_seq = fat[cluster_seq];
        int displace_of_cluster = cluster_seq - last_cluster;
        displacement += boot->BPB_BytsPerSec * (displace_of_cluster * boot->BPB_SecPerClus); 
        root = (struct DirEntry*) (mmap_info + displacement);
    }
}