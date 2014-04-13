#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "MetaDataNode.h"

#include "params.h"

typedef unsigned short BlockType;
enum BlockType { META_DATA_NODE, INDEX_NODE, DATA_NODE };

typedef struct {
	BlockType type;
	union {
		Byte data[BLOCK_SIZE - sizeof (BlockType)];
		MetaDataNode mdn;
		StoragePointer indexNode[(BLOCK_SIZE - sizeof (BlockType)) / 2];
	};
} Block;

void fillMetaBlock(Block* block, const char* name, int uid, int gid, int mode, int dataIndex);

#endif