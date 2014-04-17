#include "FileSystem.h"

static int checkAccessCondition(unsigned short fmode, int cond);
static int findAndSetFirstFreeEntry(Byte* freeSpace);
static int findFirstFreeEntry(Byte* freeSpace);
static unsigned long hash(const char *str);
static int addToOpenFileTables(const char* name, MetaDataNode* mdn, int uid, int gid, int pid, int flags);
// static PerProcessOpenFileData* findByPidAndName(int pid, const char* name);

// char* uid = "anthony";
// char* gid = "anthony";

void newFileSystem() {
	fileSystem = malloc(sizeof *fileSystem);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
		fileSystem->directory[i] = NULL;

	memset(fileSystem->freeSpace, -1, BITNSLOTS(STORAGE_SIZE));

	fileSystem->goft = NULL;

	fileSystem->processList = NULL;

	pthread_mutex_init(&fileSystem->mutex, NULL);
}

int fs_create(const char* name, int mode, int uid, int gid) {
	if (findFile(name, NULL, NULL))
		return -EEXIST;

	pthread_mutex_lock(&fileSystem->mutex);
	int mdnBlock = findAndSetFirstFreeEntry(fileSystem->freeSpace);
	int dataBlock = findAndSetFirstFreeEntry(fileSystem->freeSpace);
	// printf("mdnBlock: %d\n", mdnBlock);
	// printf("dataBlock: %d\n", dataBlock);

	fillMetaBlock(&fileSystem->storage[mdnBlock], name, uid, gid, S_IFREG | mode, dataBlock);

	unsigned long num = hash(name) % DIRECTORY_SIZE;
	fileSystem->directory[num] = g_list_prepend(fileSystem->directory[num], GUINT_TO_POINTER(mdnBlock));

	fileSystem->storage[dataBlock].type = DATA_NODE;

	pthread_mutex_unlock(&fileSystem->mutex);
	return 0;
}

PerProcessOpenFileTable* searchByPid(int pid) {
	GList* current = fileSystem->processList;

	while (current) {
		PerProcessOpenFileTable* head = current->data;
		if (head->pid == pid)
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

	return addToOpenFileTables(name, mdn, uid, gid, pid, flags);
}

static int addToOpenFileTables(const char* name, MetaDataNode* mdn, int uid, int gid, int pid, int flags) {
	pthread_mutex_lock(&fileSystem->mutex);

	GlobalOpenFileData* gofd = findByName(fileSystem->goft, name);
	if (!gofd) {
		gofd = newGlobalOpenFileData(mdn);
		fileSystem->goft = g_list_prepend(fileSystem->goft, gofd);
	} else
		gofd->fileOpenCount++;

	PerProcessOpenFileData* ppofd;
	PerProcessOpenFileTable* ppoft;
	ppoft = searchByPid(pid);
	if (!ppoft) {
		ppoft = newPerProcessOpenFileTable(pid);
		fileSystem->processList = g_list_prepend(fileSystem->processList, ppoft);
	}

	ppofd = newPerProcessOpenFileData(uid, gid, gofd, flags);
	int fd = ppoft_findFreeEntry(ppoft);
	if (fd < 0) {
		if (--gofd->fileOpenCount == 0)
			fileSystem->goft = g_list_remove(fileSystem->goft, gofd);
		pthread_mutex_unlock(&fileSystem->mutex);
		return -EMFILE;
	}
	ppoft->table[fd] = ppofd;
	// ppoft->size++;

	pthread_mutex_unlock(&fileSystem->mutex);

	return fd;
}

int fs_release(const char* name, int fd, int pid) {
	pthread_mutex_lock(&fileSystem->mutex);

	PerProcessOpenFileTable* ppoft;
	ppoft = searchByPid(pid);
	if (!ppoft) {
		pthread_mutex_unlock(&fileSystem->mutex);
		return -EBADF;
	}

	PerProcessOpenFileData* ppofd = ppoft->table[fd];
	if (!ppofd) {
		pthread_mutex_unlock(&fileSystem->mutex);
		return -EBADF;
	}

	GlobalOpenFileData* gofd = ppofd->gofd;
	if (--gofd->fileOpenCount == 0) {
		fileSystem->goft = g_list_remove(fileSystem->goft, gofd);
		free(gofd);
	}

	ppoft->table[fd] = NULL;
	free(ppofd);
	if (--ppoft->size == 0) {
		fileSystem->processList = g_list_remove(fileSystem->processList, ppoft);
		free(ppoft);
	}

	pthread_mutex_unlock(&fileSystem->mutex);
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

	if ((amode & X_OK) && !checkAccessCondition(fmode, X_OK))
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

	if (!fileSystem->directory[hash])
		BITSET(fileSystem->freeSpace, hash);

	pthread_mutex_unlock(&fileSystem->mutex);

	return 0;
}

int fs_write(int fd, const char* buf, size_t size, int pid) {
	if (!buf)
		return -EFAULT;

	PerProcessOpenFileTable* ppoft = searchByPid(pid);
	if (!ppoft)
		return -EBADF;

	PerProcessOpenFileData* ppofd = ppoft->table[fd];
	if (!ppofd)
		return -EBADF;

	if (ppofd->flags != O_WRONLY && ppofd->flags != O_RDWR)
		return -EBADF;

	pthread_mutex_lock(&fileSystem->mutex);

	MetaDataNode* mdn = ppofd->gofd->mdn;
	int numWrite = size;
	while (numWrite > 0 && mdn->size < MAX_SIZE) {
		int index = ppofd->index;
		Block* indexNode = &fileSystem->storage[mdn->dataIndex];
		Block* data;
		if (indexNode->type == INDEX_NODE)
			data = &fileSystem->storage[indexNode->indexNode[index]];
		else
			data = indexNode;

		int position = ppofd->position;
		int bytesToWrite = MIN(DATA_SIZE - position, numWrite);
		char* start = data->data + position;

		memcpy(start, buf + size - numWrite, bytesToWrite); //only ever copying start of buf

		numWrite -= bytesToWrite;
		ppofd->position += bytesToWrite;

		if (numWrite > 0 &&  ppofd->position == DATA_SIZE) {
			if (indexNode->type == DATA_NODE) {
				StoragePointer i = findAndSetFirstFreeEntry(fileSystem->freeSpace);
				Block* in = &fileSystem->storage[i];
				in->type = INDEX_NODE;
				in->indexNode[0] = mdn->dataIndex;
				mdn->dataIndex = i;
				indexNode = in;
			}

			StoragePointer j = findAndSetFirstFreeEntry(fileSystem->freeSpace);
			indexNode->indexNode[++ppofd->index] = j;
			ppofd->position = 0;
		}

		mdn->size += bytesToWrite;
		if (mdn->size == MAX_SIZE && numWrite > 0) {
			pthread_mutex_unlock(&fileSystem->mutex);
			return -EFBIG;
		}
	}

	pthread_mutex_unlock(&fileSystem->mutex);

	return size - numWrite;
}

int fs_read(int fd, char* buf, size_t size, int pid) {
	if (!buf)
		return -EFAULT;

	PerProcessOpenFileTable* ppoft = searchByPid(pid);
	if (!ppoft)
		return -EBADF;

	PerProcessOpenFileData* ppofd = ppoft->table[fd];
	if (!ppofd)
		return -EBADF;

	if (ppofd->flags != O_RDONLY && ppofd->flags != O_RDWR)
		return -EBADF;

	pthread_mutex_lock(&fileSystem->mutex);

	MetaDataNode* mdn = ppofd->gofd->mdn;
	int numRead = size;
	while (numRead > 0 && ppofd->index * DATA_SIZE + ppofd->position < mdn->size) {
		int index = ppofd->index;
		Block* indexNode = &fileSystem->storage[mdn->dataIndex];
		Block* data;
		if (indexNode->type == INDEX_NODE)
			data = &fileSystem->storage[indexNode->indexNode[index]];
		else
			data = indexNode;

		int position = ppofd->position;
		int bytesToRead = MIN(DATA_SIZE - position, numRead);
		Byte* start = data->data + position;

		memcpy(buf + size - numRead, start, bytesToRead);
		numRead -= bytesToRead;
		ppofd->position += bytesToRead;

		// position = position + DATA_SIZE;
		if (numRead > 0 && indexNode->type == INDEX_NODE && ppofd->position == DATA_SIZE) {
			ppofd->index++;
			ppofd->position = 0;
		}
	}

	pthread_mutex_unlock(&fileSystem->mutex);

	return size - numRead;
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

	while (c = (unsigned char) *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

// int main(int argc, char const *argv[])
// {
// 	printf("%lu %lu %lu %lu %lu\n", hash("test.txt"), hash("test1.txt"), hash("fffff"), hash("asdasfsdfisalbjj"), hash("mm"));
// 	return 0;
// }