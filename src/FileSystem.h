#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#include "MetaDataNode.h"
#include "Block.h"
#include "GlobalOpenFileTable.h"

#include "params.h"
#include "bitops.h"

typedef struct {
	GList* directory[DIRECTORY_SIZE];
	Block storage[STORAGE_SIZE];
	Byte freeSpace[BITNSLOTS(STORAGE_SIZE)];
	GList* goft; //globalOpenFileTable
	GList* processList; //one table for each process
	pthread_mutex_t mutex;
} FileSystem;

extern FileSystem* fileSystem;

FileSystem* newFileSystem();
int fs_create(const char* name, int mode, int uid, int gid);
int fs_unlink(const char* name, int uid, int gid);
int fs_open(const char* name, int flags, int uid, int gid, int pid);
int fs_access(const char* name, int amode, int uid, int gid);
MetaDataNode* findFile(const char* name, unsigned long* hashNum, StoragePointer* stIndex);
void freeFileSystem();

#endif