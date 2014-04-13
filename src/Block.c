#include "Block.h"

void fillMetaBlock(Block* block, const char* name, int uid, int gid, int mode, int dataIndex) {
	block->type = META_DATA_NODE;

	fillMetaDataNode(&block->mdn, name, uid, gid, mode, dataIndex);
}