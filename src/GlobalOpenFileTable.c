#include "GlobalOpenFileTable.h"

GlobalOpenFileData* newGlobalOpenFileData(MetaDataNode* mdn) {
	GlobalOpenFileData* gofd = malloc(sizeof *gofd);
	strcpy(gofd->name, mdn->name);
	gofd->mdn = mdn;
	gofd->fileOpenCount = 1;

	return gofd;
}

GlobalOpenFileData* closeGlobal(GlobalOpenFileData* gofd) {
	if (--gofd->fileOpenCount == 0) {
		free(gofd);
		return NULL;
	}

	return gofd;
}

void ppofd_move_offset(PerProcessOpenFileData* ppofd, off_t offset) {
	ppofd->index = offset / DATA_SIZE;
	ppofd->position = offset % DATA_SIZE;
}

PerProcessOpenFileData* newPerProcessOpenFileData(int uid, int gid, GlobalOpenFileData* gofd, int flags) {
	PerProcessOpenFileData* ppofd = malloc(sizeof *ppofd);
	ppofd->index = ppofd->position = 0;
	ppofd->gofd = gofd;
	ppofd->flags = flags;

	return ppofd;
}

void* findByName(GList* goft, const char* name) {
	int compareName(GlobalOpenFileData* gofd, const char* other) {
		return strcmp(gofd->name, other);
	}

	GList* l = g_list_find_custom(goft, name, (void*) compareName);
	if (!l)
		return NULL;

	return l->data;
}

PerProcessOpenFileTable* newPerProcessOpenFileTable(int pid) {
	PerProcessOpenFileTable* ppoft = malloc(sizeof *ppoft);
	ppoft->pid = pid;
	ppoft->size = 0;

	for (int i = 0; i < MAX_OPEN_FILES_PER_PROCESS; i++) {
		ppoft->table[i] = NULL;
	}

	return ppoft;
}

int ppoft_findFreeEntry(PerProcessOpenFileTable* ppoft) {
	for (int i = 0; i < MAX_OPEN_FILES_PER_PROCESS; i++) {
		if (!ppoft->table[i])
			return i;
	}

	return -1;
}

void freeGlobalOpenFileTable(GList* goft) {
	g_list_free_full(goft, free);
}