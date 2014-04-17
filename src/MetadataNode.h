#ifndef _METADATANODE_H_
#define _METADATANODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>

#include "params.h"

typedef struct {
	char name[MAX_NAME];
	size_t size;
	int uid;
	int gid;
	unsigned short fileMode;
	time_t ctime;
	time_t mtime;
	time_t atime;
	unsigned int linkCount;
	StoragePointer dataIndex;
} MetadataNode;

void displayFileInfo(MetadataNode* mdn);
void fillMetadataNode(MetadataNode* mdn, const char* name, int uid, int gid, short fileMode, int dataIndex);

#endif