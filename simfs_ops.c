/* This file contains functions that are not part of the visible "interface".
 * They are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

/* Internal helper functions first.
 */

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if((fp = fopen(filename, mode)) == NULL) {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void
closefs(FILE *fp)
{
    if(fclose(fp) != 0) {
        perror("closefs");
        exit(1);
    }
}

/* File system operations: creating, deleting, reading, and writing to files.
 */

// Signatures omitted; design as you wish.

void createfile(char *file_system_name, char *filename){
    if(strlen(filename) > 11 || strlen(filename) == 0){
        fprintf(stderr, "Error: invalid length of the filename\n");
        exit(1);
    }

    fentry files[MAXFILES];
    FILE *fp = openfs(file_system_name, "r+");
    int i;

    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    for(i = 0; i < MAXFILES; i++) {
        if(strcmp(files[i].name, filename) == 0){
            fprintf(stderr, "Error: the filename already exists\n");
            closefs(fp);
            exit(1);
        }

        if(files[i].name[0] == '\0'){
           strncpy(files[i].name, filename, 11);

            if (fseek(fp, 0, SEEK_SET) != 0) {
                fprintf(stderr, "Error: seek failed during create\n");
                closefs(fp);
                exit(1);
            }

           if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
                fprintf(stderr, "Error: write failed on init\n");
                closefs(fp);
                exit(1);
            }
           break; 
        }
        if(i == MAXFILES - 1){
            fprintf(stderr, "Error: no space to create a new file\n");
            closefs(fp);
            exit(EXIT_FAILURE);
        }
    }


    closefs(fp);
    return;
}

void deletefile(char *file_system_name, char * filename){
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    FILE *fp = openfs(file_system_name, "r+");

    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }


    int curr = -1;
    int size = -1;
    int curr_block = -1;
    for(int i = 0; i < MAXFILES; i ++){
        if(strcmp(files[i].name, filename) == 0){
            curr = i;
            size = files[i].size;
            curr_block = files[i].firstblock;

            break;
        }
    }

    if(curr == -1){
        fprintf(stderr, "Error: filename doesn't exist in the file system\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    strncpy(files[curr].name, "\0", sizeof(files[curr].name));
    files[curr].size = 0;
    files[curr].firstblock = -1;

    while(fnodes[curr_block].nextblock != -1){

    }


}

void readfile(char *file_system_name, char *filename, int start, int length){
    
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    FILE *fp = openfs(file_system_name, "r");

    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }

    int curr = -1;
    int size = -1;
    int curr_block = -1;
    for(int i = 0; i < MAXFILES; i ++){
        if(strcmp(files[i].name, filename) == 0){
            curr = i;
            size = files[i].size;
            curr_block = files[i].firstblock;
            break;
        }
    }

    if(size < length){
        fprintf(stderr, "Error: size and length invalid\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    if(curr == -1){
        fprintf(stderr, "Error: filename doesn't exist in the file system\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    if(start > size){
        fprintf(stderr, "Error: start > size\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    if(curr_block == -1){
        closefs(fp);    
        return;
    }

    if(start > BLOCKSIZE){
        start = start % BLOCKSIZE;
        int literal = start / BLOCKSIZE;
        for(int i = 0; i < literal; i ++){
            curr_block = fnodes[curr_block].nextblock; 
        }
    }

    if(fseek(fp, curr_block * BLOCKSIZE + start, SEEK_SET) != 0){
        fprintf(stderr, "Error: seek failed during write\n");
        closefs(fp);
        exit(1);
    }

    char block[BLOCKSIZE + 1];     
    int curr_space = BLOCKSIZE - start;  
    if(curr_space > length){
        curr_space = length;
    }


    int remain = length;
    int read = 0;

    while(remain != 0){
        read = fread(block, sizeof(char), curr_space, fp);    

        remain -= read;    

        if(read != curr_space && ferror(stdin)){   
            fprintf(stderr, "Error: fread failed\n");
            closefs(fp);
            exit(1);
        }

        if(fwrite(block, sizeof(char), read, stdout)!= read) {   
            fprintf(stderr, "Error: fwrite failed\n");
            closefs(fp);
            exit(1);
        }

        if(remain == 0) {     
            break;     
        }
        

        if(fnodes[curr_block].nextblock == -1){   
            return;
        }
        
        curr_block = fnodes[curr_block].nextblock;

        if(fseek(fp, curr_block * BLOCKSIZE, SEEK_SET) != 0){
            fprintf(stderr, "Error: seek failed during write\n");
            closefs(fp);
            exit(1);
        }

        if(remain > BLOCKSIZE){
            curr_space = BLOCKSIZE;     
        }
        else{
            curr_space = remain;     
        }

    }
    printf("\n");
    closefs(fp);

}

int find_empty_block(){
    fnode fnodes[MAXBLOCKS];
    int ans = 0;
    for(int i = 0; i < MAXBLOCKS; i++){
        if(fnodes[i].blockindex < 0){
            ans = i;
        }
    }
    return ans;
}


void writefile(char *file_system_name, char *filename, int start, int length){

    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    FILE *fp = openfs(file_system_name, "r+");

    if ((fread(files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }

    int file_index = -1;                 
    int size = -1;
    int file_block = -1;              
    for(int i = 0; i < MAXFILES; i ++){
        if(strcmp(files[i].name, filename) == 0){
            file_index = i;
            size = files[i].size;
            file_block = files[i].firstblock;
            break;
        }
    }

    if(file_index == -1){
        fprintf(stderr, "Error: filename doesn't exist in the file system\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    if(start > size){
        fprintf(stderr, "Error: start > size\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    int len = strlen(filename);
    if(len > 11){
        fprintf(stderr, "Error: too many characters in the parameter filename\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }

    int writing_space = start + length;
    int curr_block = file_block; 
    int unused_block = 0;
    int used_block = 0;
    while (curr_block > 0)
    {
        used_block ++;
        curr_block = fnodes[curr_block].nextblock;
    }

    for (int i = 0; i < MAXBLOCKS; i++)
    {
        if(fnodes[i].blockindex < 0){
            unused_block ++;
        }
    }
    writing_space -= used_block * BLOCKSIZE;
    writing_space -= unused_block * BLOCKSIZE;

    if(writing_space > 0){     
        fprintf(stderr, "Error: not enough space to write\n");
        closefs(fp);
        exit(EXIT_FAILURE);
    }


    if(file_block == -1){
        for(int i = 0; i < MAXBLOCKS; i++){
            if(fnodes[i].blockindex < 0){    
                files[file_index].firstblock = i;
                fnodes[i].blockindex = i;
                file_block = fnodes[i].blockindex;     
                break;
            }
        }
    }

   int start_cpy = start;    

    if(start > BLOCKSIZE){
        int literal = start / BLOCKSIZE;
        start = start % BLOCKSIZE;
        for(int i = 0; i < literal; i ++){
            file_block = fnodes[file_block].nextblock; 
        }
    }

    if(fseek(fp, file_block * BLOCKSIZE + start, SEEK_SET) != 0){
        fprintf(stderr, "Error: seek failed during write\n");
        closefs(fp);
        exit(1);
    }

    char block[BLOCKSIZE + 1];
    int space = BLOCKSIZE - start;  
    if(space > length){
        space = length;
    }

    int remain = length;
    int read = 0;

    if(fseek(fp, 0, SEEK_SET) != 0){
            fprintf(stderr, "Error: seek failed during write\n");
            closefs(fp);
            exit(1);
    }

    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: write failed on writing, when write the fentry back\n");
        closefs(fp);
        exit(1);
    }

    if(fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS) {
        fprintf(stderr, "Error: write failed on writing, when write the fnode back\n");
        closefs(fp);
        exit(1);
    }

    while(remain != 0){
        read = fread(block, sizeof(char), space, stdin);    
        remain -= read; 

        if(read != space && ferror(stdin) != 0){ 
            fprintf(stderr, "Error: fread failed\n");
            closefs(fp);
            exit(1);
        }

        if(fwrite(block, sizeof(char), read, fp)!= read) {   
            fprintf(stderr, "Error: fwrite failed\n");
            closefs(fp);
            exit(1);
        }

        if(remain == 0) {    
            char zerobuf[BLOCKSIZE] = {0};   
            int bytes_to_write = (BLOCKSIZE - ((start_cpy + length) % BLOCKSIZE)) % BLOCKSIZE;
            if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, fp) < 1) {
                fprintf(stderr, "Error: write failed on init\n");
                closefs(fp);
                exit(1);
            }
            break;       
        }
        

        if(fnodes[file_block].nextblock == -1){   
            for(int i = 0; i < MAXBLOCKS; i++){
                if(fnodes[i].blockindex < 0){
                    fnodes[file_block].nextblock = i;
                    fnodes[i].blockindex = i;
                    break;
                }
            }
        }
        
        file_block = fnodes[file_block].nextblock;
        if(fseek(fp, file_block * BLOCKSIZE, SEEK_SET) != 0){
            fprintf(stderr, "Error: seek failed during write\n");
            closefs(fp);
            exit(1);
        }

        if(remain > BLOCKSIZE){
            space = BLOCKSIZE;    
        }
        else{
            space = remain;      
        }

    }

    if(files[file_index].size < start_cpy + length){
        files[file_index].size = start_cpy + length;
    }
        

    closefs(fp);
}
    



    

    
