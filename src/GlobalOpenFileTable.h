#ifndef _GLOBALOPENFILETABLE_H_
#define _GLOBALOPENFILETABLE_H_

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "MetaDataNode.h"
#include "Block.h"

#include "params.h"

// enum FileMode { X = 1, W, XW, R, RX, RW, RWX};

typedef struct {
	char name[MAX_NAME];
	MetaDataNode* mdn;
	unsigned int fileOpenCount;
} GlobalOpenFileData;

typedef struct {
	short index;
	short position;
	int flags;
	GlobalOpenFileData* gofd;
} PerProcessOpenFileData;

typedef struct {
	PerProcessOpenFileData* table[MAX_OPEN_FILES_PER_PROCESS];
	int size;
	int pid;
} PerProcessOpenFileTable;

PerProcessOpenFileData* newPerProcessOpenFileData(int uid, int gid, GlobalOpenFileData* gofd, int flags);
PerProcessOpenFileTable* newPerProcessOpenFileTable(int pid);
GlobalOpenFileData* newGlobalOpenFileData(MetaDataNode* mdn);
void* findByName(GList*, const char* name);
// void* findByPid(GList* ppoft, int pid);
void freeGlobalOpenFileTable(GList* goft);
PerProcessOpenFileTable* newPerProcessOpenFileTable(int pid);
int ppoft_findFreeEntry(PerProcessOpenFileTable* ppoft);
void ppofd_move_offset(PerProcessOpenFileData* ppofd, off_t offset);
PerProcessOpenFileTable* searchByPid(int pid);


#endif