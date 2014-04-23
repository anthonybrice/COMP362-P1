/**
* Name: Anthony Brice
* Lab/task: Project 1 Task 5
* Date: 04/23/14
**/

#ifndef _PARAMS_H_
#define _PARAMS_H_

#define MAX_NAME 64
#define BLOCK_SIZE 256
#define STORAGE_SIZE 65535
#define DIRECTORY_SIZE 65551
#define MAX_OPEN_FILES_PER_PROCESS 64

// Must be large enough to hold entries 0 through STORAGE_SIZE inclusive. The last value (STORAGE_SIZE) will be used as NULL
typedef unsigned short StoragePointer;
typedef unsigned char Byte;

#define NULL_VALUE STORAGE_SIZE

#endif