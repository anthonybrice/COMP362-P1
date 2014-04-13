#ifndef _GLOBALOPENFILETABLE_H_
#define _GLOBALOPENFILETABLE_H_

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
// #include <fcntl.h>

#include "MetaDataNode.h"

#include "params.h"

enum FileMode { X = 1, W, XW, R, RX, RW, RWX};

typedef struct GlobalOpenFileData {
	char name[MAX_NAME];
	MetaDataNode* mdn;
	StoragePointer dataLocation;
	unsigned int fileOpenCount;
} GlobalOpenFileData;

typedef struct PerProcessOpenFileData {
	StoragePointer position;
	unsigned int pid;
	Byte fileMode;
	GlobalOpenFileData* gofd;
} PerProcessOpenFileData;

PerProcessOpenFileData* newPerProcessOpenFileData(unsigned int pid, int uid, int gid,  MetaDataNode* mdn);
GlobalOpenFileData* newGlobalOpenFileData(MetaDataNode* mdn);
void* findByName(GList* goft, char* name);
void* findByPid(GList* ppoft, int pid);
void freeGlobalOpenFileTable(GList* goft);


#endif