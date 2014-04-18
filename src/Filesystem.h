#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#include "MetadataNode.h"
#include "Block.h"
#include "GlobalOpenFileTable.h"

#include "params.h"
#include "bitops.h"

#define MAX_SIZE STORAGE_SIZE * DATA_SIZE

typedef struct {
	GList* directory[DIRECTORY_SIZE];
	Block storage[STORAGE_SIZE];
	Byte freeSpace[BITNSLOTS(STORAGE_SIZE)];
	GList* goft; //globalOpenFileTable
	GList* processList; //one table for each process
	pthread_mutex_t mutex;
} Filesystem;

extern Filesystem* fs;

void fs_init();
int fs_create(const char* name, int mode, int uid, int gid);
int fs_unlink(const char* name, int uid, int gid);
int fs_open(const char* name, int flags, int uid, int gid, int pid);
int fs_access(const char* name, int amode, int uid, int gid);
int fs_release(const char* name, int fd, int pid);
int fs_read(int fd, char* buf, size_t size, int pid);
int fs_write(int fd, const char* buf, size_t size, int pid);
PerProcessOpenFileTable* fs_find_ppoft(int pid);
int fs_utimens(const char* name, const struct timespec ts[2]);
int fs_truncate(const char* name, off_t size);
int fs_chmod(const char* name, mode_t mode, int uid);

MetadataNode* find_file(const char* name, unsigned long* hashNum, StoragePointer* stIndex);
void fs_free();

#endif