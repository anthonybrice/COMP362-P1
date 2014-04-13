#include "FileSystem.h"

extern FileSystem* fileSystem;

int checkAccessCondition(unsigned short fmode, int cond);
int findAndSetFirstFreeEntry(Byte* freeSpace);
int findFirstFreeEntry(Byte* freeSpace);
MetaDataNode* findFile(const char* name, unsigned long* hashNum, StoragePointer* stIndex);
unsigned long hash(const char *str);

// char* uid = "anthony";
// char* gid = "anthony";

FileSystem* newFileSystem() {
	fileSystem = malloc(sizeof *fileSystem);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
		fileSystem->directory[i] = NULL;

	// int test = BITNSLOTS(BIT_VECTOR_SIZE);
	// printf("gonna go %d\n", test);
	// for (int i = 0; i < BIT_VECTOR_SIZE; i++)
		// BITSET(fileSystem->freeSpace, i);
	memset(fileSystem->freeSpace, -1, BITNSLOTS(BIT_VECTOR_SIZE));

	fileSystem->goft = NULL;

	fileSystem->ppoft = NULL;

	pthread_mutex_init(&fileSystem->mutex, NULL);

	return fileSystem;
}

int fs_create(const char* name, int mode, int uid, int gid) {
	if (findFile(name, NULL, NULL))
		return -EEXIST;

	pthread_mutex_lock(&fileSystem->mutex);
	int mdnBlock = findAndSetFirstFreeEntry(fileSystem->freeSpace);
	int dataBlock = findAndSetFirstFreeEntry(fileSystem->freeSpace);
	printf("mdnBlock: %d\n", mdnBlock);
	printf("dataBlock: %d\n", dataBlock);

	fillMetaBlock(&fileSystem->storage[mdnBlock], name, uid, gid, S_IFREG | mode, dataBlock);
	// fillMetaBlock(&fileSystem->storage[mdnBlock], name, uid, gid, S_IFREG | 0777, dataBlock);

	unsigned long num = hash(name) % DIRECTORY_SIZE;
	fileSystem->directory[num] = g_list_prepend(fileSystem->directory[num], GUINT_TO_POINTER(mdnBlock));

	fileSystem->storage[dataBlock].type = DATA_NODE;

	pthread_mutex_unlock(&fileSystem->mutex);
	return 0;
}

int fs_open(const char* name, int flags, int uid, int gid) {
	MetaDataNode* mdn = findFile(name, NULL, NULL);
	if (!mdn)
		return -ENOENT;

	unsigned short fmode = mdn->fileMode;

	if ((flags & O_RDONLY) && !checkAccessCondition(fmode, R_OK))
		return -EACCES;
	else if ((flags & O_WRONLY) && !checkAccessCondition(fmode, W_OK))
		return -EACCES;
	else if ((flags & O_RDWR) && (!checkAccessCondition(fmode, R_OK) || !checkAccessCondition(fmode, R_OK)))
		return -EACCES;

	return 0;
}

int fs_access(const char* name, int amode, int uid, int gid) {
	if (!strcmp(name, "/"))
		return 0;

	MetaDataNode* mdn = findFile(name, NULL, NULL);
	if (!mdn)
		return -ENOENT;

	unsigned short fmode = mdn->fileMode;

	if ((amode & R_OK) && !checkAccessCondition(fmode, R_OK))
		return -EACCES;

	if ((amode & W_OK) && !checkAccessCondition(fmode, W_OK))
		return -EACCES;

	if ((amode & X_OK) && !checkAccessCondition(fmode, W_OK))
		return -EACCES;

	return 0;
}

int findAndSetFirstFreeEntry(Byte* freeSpace) {
	int i = findFirstFreeEntry(freeSpace);
	if (i < 0) {
		fprintf(stderr, "Out of space in storage.\n");
		exit(-1);
	}
	BITCLEAR(freeSpace, i);

	return i;
}

int findFirstFreeEntry(Byte* freeSpace) {
	for (int i = 0; i < BIT_VECTOR_SIZE; i++) {
		if (BITTEST(freeSpace, i))
			return i;
	}

	return -1;
}

int checkAccessCondition(unsigned short fmode, int cond) {
	return fmode & cond || fmode & (cond << 3) || fmode & (cond << 6);
}

int fs_unlink(const char* name, int uid, int gid) {
	pthread_mutex_lock(&fileSystem->mutex);

	unsigned long hash = 0;
	StoragePointer stIndex = 0;
	MetaDataNode* mdn = findFile(name, &hash, &stIndex);
	if (!mdn)
		return -ENOENT;

	StoragePointer p = mdn->dataIndex;

	if (fileSystem->storage[p].type == INDEX_NODE) {
		StoragePointer q;
		for (int i = 0; i < (BLOCK_SIZE - sizeof (BlockType)) / 2 && (q = fileSystem->storage[p].indexNode[i]) != NULL_VALUE; i++) {
			BITSET(fileSystem->freeSpace, q);
		}
	}
	BITSET(fileSystem->freeSpace, p);

	fileSystem->directory[hash] = g_list_remove(fileSystem->directory[hash], GUINT_TO_POINTER(stIndex));
	BITSET(fileSystem->freeSpace, hash);

	pthread_mutex_unlock(&fileSystem->mutex);

	return 0;
}

MetaDataNode* findFile(const char* name, unsigned long* hashNum, StoragePointer* stIndex) {
	unsigned long hashIndex = hash(name) % DIRECTORY_SIZE;
	if (hashNum)
		*hashNum = hashIndex;

	GList* l = fileSystem->directory[hashIndex];
	while (l) {
		MetaDataNode* mdn = &fileSystem->storage[GPOINTER_TO_UINT(l->data)].mdn;
		if (!strcmp(mdn->name, name)) {
			if (stIndex)
				*stIndex = GPOINTER_TO_UINT(l->data);

			return mdn;
		}
		l = l->next;
	}

	return NULL;
}

void freeFileSystem() {
	for (int i = 0; i < DIRECTORY_SIZE; i++) {
		g_list_free(fileSystem->directory[i]);
	}

	free(fileSystem);
}

// djb2: http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(const char *str) {
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
};