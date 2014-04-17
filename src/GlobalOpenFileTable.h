#ifndef _GLOBALOPENFILETABLE_H_
#define _GLOBALOPENFILETABLE_H_

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "MetadataNode.h"

#include "params.h"

typedef struct {
	char name[MAX_NAME];
	MetadataNode* mdn;
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

PerProcessOpenFileData* ppofd_init(int uid, int gid, GlobalOpenFileData* gofd, int flags);
PerProcessOpenFileTable* ppoft_init(int pid);
GlobalOpenFileData* gofd_init(MetadataNode* mdn);
GlobalOpenFileData* goft_find_by_name(GList*, const char* name);
PerProcessOpenFileTable* ppoft_init(int pid);
int ppoft_find_free_entry(PerProcessOpenFileTable* ppoft);
void ppofd_move_offset(PerProcessOpenFileData* ppofd, off_t offset);


#endif