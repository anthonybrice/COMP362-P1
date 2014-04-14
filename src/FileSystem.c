#include "FileSystem.h"

extern FileSystem* fileSystem;

static int checkAccessCondition(unsigned short fmode, int cond);
static int findAndSetFirstFreeEntry(Byte* freeSpace);
static int findFirstFreeEntry(Byte* freeSpace);
static unsigned long hash(const char *str);
editOpenFileTables(const char* name, int uid, int gid, int pid);
// static PerProcessOpenFileData* findByPidAndName(int pid, const char* name);

// char* uid = "anthony";
// char* gid = "anthony";

FileSystem* newFileSystem() {
	fileSystem = malloc(sizeof *fileSystem);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
		fileSystem->directory[i] = NULL;

	memset(fileSystem->freeSpace, -1, BITNSLOTS(STORAGE_SIZE));

	fileSystem->goft = NULL;

	fileSystem->ppoft = NULL;

	pthread_mutex_init(&fileSystem->mutex, NULL);
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

	unsigned long num = hash(name) % DIRECTORY_SIZE;
	fileSystem->directory[num] = g_list_prepend(fileSystem->directory[num], GUINT_TO_POINTER(mdnBlock));

	fileSystem->storage[dataBlock].type = DATA_NODE;

	pthread_mutex_unlock(&fileSystem->mutex);
	return 0;
}

static GList* searchByPid(int pid) {
	GList* current = fileSystem->processList;

	while (current) {
		GList* head = current->data;
		if (head->data->pid == pid)
			return head;

		current = current->next;
	}

	return NULL;
}

int fs_open(const char* name, int flags, int uid, int gid, int pid) {
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

	editOpenFileTables(name, uid, gid, pid);

	return 0;
}

static void editOpenFileTables(const char* name, int uid, int gid, int pid) {
	pthread_mutex_lock(fileSystem->mutex);

	GlobalOpenFileData* gofd = findByName(fileSystem->goft, name);
	if (!gofd) {
		gofd = newGlobalOpenFileData(mdn);
		fileSystem->goft = g_list_prepend(fileSystem->goft, gofd);
	}

	PerProcessOpenFileData* ppofd;
	GList* ppoft; // perProcessOpenFileTable
	ppoft = searchByPid(fileSystem->processTable, pid);
	if (!ppoft) {
		// then this process has no other open files.
		ppofd = newPerProcessOpenFileData(pid, uid, gid, mdn, gofd);
		ppoft = g_list_prepend(ppoft, ppofd);
		fileSystem->processTable = g_list_prepend(fileSystem->processTable, ppoft);
	} else {
		ppofd = findByName(ppoft, name);
		if (!ppofd) {
			// then this process does not already have the file open.
			ppofd = newPerProcessOpenFileData(pid, uid, gid, mdn, gofd);
			fileSystem->processTable = g_list_remove_link(fileSystem->processTable, ppoft);
			ppoft = g_list_prepend(ppoft, ppofd);
			fileSystem->processTable = g_list_prepend(fileSystem->processTable, ppoft);
		} // else we do nothing. The process already has this file open once. If we add another table, how will we know which table the process requests later?
	}

	pthread_mutex_unlock(fileSystem->mutex);
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

static int findAndSetFirstFreeEntry(Byte* freeSpace) {
	int i = findFirstFreeEntry(freeSpace);
	if (i < 0) {
		fprintf(stderr, "Out of space in storage.\n");
		exit(-1);
	}
	BITCLEAR(freeSpace, i);

	return i;
}

static int findFirstFreeEntry(Byte* freeSpace) {
	for (int i = 0; i < STORAGE_SIZE; i++) {
		if (BITTEST(freeSpace, i))
			return i;
	}

	return -1;
}

static int checkAccessCondition(unsigned short fmode, int cond) {
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
static unsigned long hash(const char *str) {
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
};