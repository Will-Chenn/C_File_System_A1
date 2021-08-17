#include <stdio.h>
#include "simfstypes.h"

/* File system operations */
void printfs(char *);
void initfs(char *);

/* Internal functions */
FILE *openfs(char *filename, char *mode);
void closefs(FILE *fp);
void createfile(char *file_system_name, char *filename);
void writefile(char *file_system_name, char *filename, int start, int length);
void readfile(char *file_system_name, char *filename, int start, int length);
void deletefile(char *file_system_name, char * filename);