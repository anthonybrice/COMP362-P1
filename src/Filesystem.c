#include "Filesystem.h"

static int file_access_ok(unsigned short fmode, int cond);
static int find_and_set_block_filled();
static int find_empty_block();
static unsigned long hash(const char *str);
static int file_tables_add(const char* name, MetadataNode* mdn, int uid, int gid, int pid, int flags);
// static PerProcessOpenFileData* findByPidAndName(int pid, const char* name);

// char* uid = "anthony";
// char* gid = "anthony";

void fs_init() {
	fs = malloc(sizeof *fs);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
		fs->directory[i] = NULL;

	memset(fs->freeSpace, -1, BITNSLOTS(STORAGE_SIZE));

	fs->goft = NULL;

	fs->processList = NULL;

	pthread_mutex_init(&fs->mutex, NULL);
}

int fs_create(const char* name, int mode, int uid, int gid) {
	if (find_file(name, NULL, NULL))
		return -EEXIST;

	pthread_mutex_lock(&fs->mutex);
	int mdnBlock = find_and_set_block_filled();
	int dataBlock = find_and_set_block_filled();
	// printf("mdnBlock: %d\n", mdnBlock);
	// printf("dataBlock: %d\n", dataBlock);

	block_fill_meta(&fs->storage[mdnBlock], name, uid, gid, S_IFREG | mode, dataBlock);

	unsigned long num = hash(name) % DIRECTORY_SIZE;
	fs->directory[num] = g_list_prepend(fs->directory[num], GUINT_TO_POINTER(mdnBlock));

	fs->storage[dataBlock].type = DATA_NODE;

	pthread_mutex_unlock(&fs->mutex);
	return 0;
}

PerProcessOpenFileTable* fs_find_ppoft(int pid) {
	GList* current = fs->processList;

	while (current) {
		PerProcessOpenFileTable* head = current->data;
		if (head->pid == pid)
			return head;

		current = current->next;
	}

	return NULL;
}

int fs_open(const char* name, int flags, int uid, int gid, int pid) {
	MetadataNode* mdn = find_file(name, NULL, NULL);
	if (!mdn)
		return -ENOENT;

	unsigned short fmode = mdn->fileMode;

	if ((flags & O_RDONLY) && !file_access_ok(fmode, R_OK))
		return -EACCES;
	else if ((flags & O_WRONLY) && !file_access_ok(fmode, W_OK))
		return -EACCES;
	else if ((flags & O_RDWR) && (!file_access_ok(fmode, R_OK) || !file_access_ok(fmode, R_OK)))
		return -EACCES;

	return file_tables_add(name, mdn, uid, gid, pid, flags);
}

static int file_tables_add(const char* name, MetadataNode* mdn, int uid, int gid, int pid, int flags) {
	pthread_mutex_lock(&fs->mutex);

	GlobalOpenFileData* gofd = goft_find_by_name(fs->goft, name);
	if (!gofd) {
		gofd = gofd_init(mdn);
		fs->goft = g_list_prepend(fs->goft, gofd);
	} else
		gofd->fileOpenCount++;

	PerProcessOpenFileData* ppofd;
	PerProcessOpenFileTable* ppoft;
	ppoft = fs_find_ppoft(pid);
	if (!ppoft) {
		ppoft = ppoft_init(pid);
		fs->processList = g_list_prepend(fs->processList, ppoft);
	}

	ppofd = ppofd_init(uid, gid, gofd, flags);
	int fd = ppoft_add_data(ppoft, ppofd);
	if (fd < 0) {
		close_gofd(gofd);
		close_ppoft(ppoft);
		free(ppofd);

		pthread_mutex_unlock(&fs->mutex);
		return -EMFILE;
	}

	pthread_mutex_unlock(&fs->mutex);

	return fd;
}

static void close_ppoft(ppoft) {
	if (--ppoft->size == 0) {
		fs->processList = g_list_remove(fs->processList, ppoft);
		free(ppoft);
	}
}

static void close_gofd(gofd) {
	if (--gofd->fileOpenCount == 0) {
		fs->goft = g_list_remove(fs->goft, gofd);
		free(gofd);
	}
}

int fs_release(const char* name, int fd, int pid) {
	pthread_mutex_lock(&fs->mutex);

	PerProcessOpenFileTable* ppoft;
	ppoft = fs_find_ppoft(pid);
	if (!ppoft) {
		pthread_mutex_unlock(&fs->mutex);
		return -EBADF;
	}

	PerProcessOpenFileData* ppofd = ppoft->table[fd];
	if (!ppofd) {
		pthread_mutex_unlock(&fs->mutex);
		return -EBADF;
	}

	GlobalOpenFileData* gofd = ppofd->gofd;
	if (--gofd->fileOpenCount == 0) {
		fs->goft = g_list_remove(fs->goft, gofd);
		free(gofd);
	}

	ppoft->table[fd] = NULL;
	free(ppofd);
	if (--ppoft->size == 0) {
		fs->processList = g_list_remove(fs->processList, ppoft);
		free(ppoft);
	}

	pthread_mutex_unlock(&fs->mutex);
	return 0;
}

int fs_access(const char* name, int amode, int uid, int gid) {
	if (!strcmp(name, "/"))
		return 0;

	MetadataNode* mdn = find_file(name, NULL, NULL);
	if (!mdn)
		return -ENOENT;

	unsigned short fmode = mdn->fileMode;

	if ((amode & R_OK) && !file_access_ok(fmode, R_OK))
		return -EACCES;

	if ((amode & W_OK) && !file_access_ok(fmode, W_OK))
		return -EACCES;

	if ((amode & X_OK) && !file_access_ok(fmode, X_OK))
		return -EACCES;

	return 0;
}

static int find_and_set_block_filled() {
	Byte* fsp = fs->freeSpace;

	int i = find_empty_block();
	if (i < 0) {
		fprintf(stderr, "Out of space in storage.\n");
		exit(-1);
	}
	BITCLEAR(fsp, i);

	return i;
}

static int find_empty_block() {
	for (int i = 0; i < STORAGE_SIZE; i++) {
		if (BITTEST(fs->freeSpace, i))
			return i;
	}

	return -1;
}

static int file_access_ok(unsigned short fmode, int cond) {
	return fmode & cond || fmode & (cond << 3) || fmode & (cond << 6);
}

int fs_unlink(const char* name, int uid, int gid) {
	pthread_mutex_lock(&fs->mutex);

	unsigned long hash = 0;
	StoragePointer stIndex = 0;
	MetadataNode* mdn = find_file(name, &hash, &stIndex);
	if (!mdn)
		return -ENOENT;

	StoragePointer p = mdn->dataIndex;

	if (fs->storage[p].type == INDEX_NODE) {
		StoragePointer q;
		for (int i = 0; i < (BLOCK_SIZE - sizeof (BlockType)) / 2 && (q = fs->storage[p].indexNode[i]) != NULL_VALUE; i++) {
			BITSET(fs->freeSpace, q);
		}
	}
	BITSET(fs->freeSpace, p);

	fs->directory[hash] = g_list_remove(fs->directory[hash], GUINT_TO_POINTER(stIndex));

	if (!fs->directory[hash])
		BITSET(fs->freeSpace, hash);

	pthread_mutex_unlock(&fs->mutex);

	return 0;
}

int fs_write(int fd, const char* buf, size_t size, int pid) {
	if (!buf)
		return -EFAULT;

	PerProcessOpenFileTable* ppoft = fs_find_ppoft(pid);
	if (!ppoft)
		return -EBADF;

	PerProcessOpenFileData* ppofd = ppoft->table[fd];
	if (!ppofd)
		return -EBADF;

	if (ppofd->flags != O_WRONLY && ppofd->flags != O_RDWR)
		return -EBADF;

	pthread_mutex_lock(&fs->mutex);

	MetadataNode* mdn = ppofd->gofd->mdn;
	int numWrite = size;
	while (numWrite > 0 && mdn->size < MAX_SIZE) {
		int index = ppofd->index;
		Block* indexNode = &fs->storage[mdn->dataIndex];
		Block* data;
		if (indexNode->type == INDEX_NODE)
			data = &fs->storage[indexNode->indexNode[index]];
		else
			data = indexNode;

		int position = ppofd->position;
		int bytesToWrite = MIN(DATA_SIZE - position, numWrite);
		Byte* start = data->data + position;

		memcpy(start, buf + size - numWrite, bytesToWrite); //only ever copying start of buf

		numWrite -= bytesToWrite;
		ppofd->position += bytesToWrite;

		if (numWrite > 0 &&  ppofd->position == DATA_SIZE) {
			if (indexNode->type == DATA_NODE) {
				StoragePointer i = find_and_set_block_filled();
				Block* in = &fs->storage[i];
				in->type = INDEX_NODE;
				in->indexNode[0] = mdn->dataIndex;
				mdn->dataIndex = i;
				indexNode = in;
			}

			StoragePointer j = find_and_set_block_filled();
			indexNode->indexNode[++ppofd->index] = j;
			ppofd->position = 0;
		}

		mdn->size += bytesToWrite;
		if (mdn->size == MAX_SIZE && numWrite > 0) {
			pthread_mutex_unlock(&fs->mutex);
			return -EFBIG;
		}
	}

	pthread_mutex_unlock(&fs->mutex);

	return size - numWrite;
}

int fs_read(int fd, char* buf, size_t size, int pid) {
	if (!buf)
		return -EFAULT;

	PerProcessOpenFileTable* ppoft = fs_find_ppoft(pid);
	if (!ppoft)
		return -EBADF;

	PerProcessOpenFileData* ppofd = ppoft->table[fd];
	if (!ppofd)
		return -EBADF;

	if (ppofd->flags != O_RDONLY && ppofd->flags != O_RDWR)
		return -EBADF;

	pthread_mutex_lock(&fs->mutex);

	MetadataNode* mdn = ppofd->gofd->mdn;
	int numRead = size;
	while (numRead > 0 && ppofd->index * DATA_SIZE + ppofd->position < mdn->size) {
		int index = ppofd->index;
		Block* indexNode = &fs->storage[mdn->dataIndex];
		Block* data;
		if (indexNode->type == INDEX_NODE)
			data = &fs->storage[indexNode->indexNode[index]];
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

	pthread_mutex_unlock(&fs->mutex);

	return size - numRead;
}

MetadataNode* find_file(const char* name, unsigned long* hashNum, StoragePointer* stIndex) {
	unsigned long hashIndex = hash(name) % DIRECTORY_SIZE;
	if (hashNum)
		*hashNum = hashIndex;

	GList* l = fs->directory[hashIndex];
	while (l) {
		MetadataNode* mdn = &fs->storage[GPOINTER_TO_UINT(l->data)].mdn;
		if (!strcmp(mdn->name, name)) {
			if (stIndex)
				*stIndex = GPOINTER_TO_UINT(l->data);

			return mdn;
		}
		l = l->next;
	}

	return NULL;
}

void freeFilesystem() {
	for (int i = 0; i < DIRECTORY_SIZE; i++) {
		g_list_free(fs->directory[i]);
	}

	free(fs);
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