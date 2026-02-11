#include "types.h" 

void* memset(void*, int, uint);
int memcmp(const void*, const void*, uint);
void* memmove(void*, const void*, uint);
void* memcpy(void*, const void*, uint);
int strncmp(const char*, const char*, uint);
char* strncpy(char*, const char*, int);
char* safestrcpy(char*, const char*, int);
int strlen(const char*);
char* strcat(char*, const char*);