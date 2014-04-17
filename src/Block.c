#include "Block.h"

void block_fill_meta(Block* block, const char* name, int uid, int gid, int mode, int dataIndex) {
	block->type = META_DATA_NODE;

	fillMetadataNode(&block->mdn, name, uid, gid, mode, dataIndex);
}