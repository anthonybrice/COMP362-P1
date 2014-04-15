#ifndef _GLOBALOPENFILETABLE_H_
#define _GLOBALOPENFILETABLE_H_

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "MetaDataNode.h"

#include "params.h"

enum FileMode { X = 1, W, XW, R, RX, RW, RWX};

typedef struct {
	char name[MAX_NAME];
	MetaDataNode* mdn;
	StoragePointer dataLocation;
	unsigned int fileOpenCount;
} GlobalOpenFileData;

typedef struct {
	int fd; // fileDescriptor, unique id used to index the requested ppofd in its ppoft
	StoragePointer position;
	int flags;
	GlobalOpenFileData* gofd;
} PerProcessOpenFileData;

typedef struct {
	PerProcessOpenFileData* table[MAX_OPEN_FILES_PER_PROCESS];
	int size;
	int pid;
} PerProcessOpenFileTable;

PerProcessOpenFileData* newPerProcessOpenFileData(int uid, int gid,  MetaDataNode* mdn, GlobalOpenFileData* gofd, int flags);
PerProcessOpenFileTable* newPerProcessOpenFileTable(int pid);
GlobalOpenFileData* newGlobalOpenFileData(MetaDataNode* mdn);
void* findByName(GList*, const char* name);
void* findByPid(GList* ppoft, int pid);
void freeGlobalOpenFileTable(GList* goft);
PerProcessOpenFileTable* newPerProcessOpenFileTable(int pid);
int ppoft_findFreeEntry(PerProcessOpenFileTable* ppoft);

#endif