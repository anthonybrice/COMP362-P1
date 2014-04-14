#ifndef _PARAMS_H_
#define _PARAMS_H_

#define MAX_NAME 64
#define BLOCK_SIZE 256
#define STORAGE_SIZE 65535
#define DIRECTORY_SIZE 65551

// Must be large enough to hold entries 0 through STORAGE_SIZE inclusive. The last value (STORAGE_SIZE) will be used as NULL
typedef unsigned short StoragePointer;
typedef unsigned char Byte;

#define NULL_VALUE STORAGE_SIZE

#endif