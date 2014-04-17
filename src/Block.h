#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "MetadataNode.h"

#include "params.h"

typedef unsigned short BlockType;
enum BlockType { META_DATA_NODE, INDEX_NODE, DATA_NODE };

#define DATA_SIZE (BLOCK_SIZE - sizeof (BlockType))

typedef struct {
	BlockType type;
	union {
		Byte data[DATA_SIZE];
		MetadataNode mdn;
		StoragePointer indexNode[DATA_SIZE / 2];
	};
} Block;

void block_fill_meta(Block* block, const char* name, int uid, int gid, int mode, int dataIndex);

#endif