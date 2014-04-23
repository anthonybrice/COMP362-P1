/**
* Name: Anthony Brice
* Lab/task: Project 1 Task 5
* Date: 04/23/14
**/

#include "GlobalOpenFileTable.h"

GlobalOpenFileData* gofd_init(MetadataNode* mdn) {
	GlobalOpenFileData* gofd = malloc(sizeof *gofd);
	strcpy(gofd->name, mdn->name);
	gofd->mdn = mdn;
	gofd->fileOpenCount = 1;

	return gofd;
}

GlobalOpenFileData* gofd_close(GlobalOpenFileData* gofd) {
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

PerProcessOpenFileData* ppofd_init(int uid, int gid, GlobalOpenFileData* gofd, int flags) {
	PerProcessOpenFileData* ppofd = malloc(sizeof *ppofd);
	ppofd->index = ppofd->position = 0;
	ppofd->gofd = gofd;
	ppofd->flags = flags;

	return ppofd;
}

GlobalOpenFileData* goft_find_by_name(GList* goft, const char* name) {
	int compareName(GlobalOpenFileData* gofd, const char* other) {
		return strcmp(gofd->name, other);
	}

	GList* l = g_list_find_custom(goft, name, (void*) compareName);
	if (!l)
		return NULL;

	return l->data;
}

PerProcessOpenFileTable* ppoft_init(int pid) {
	PerProcessOpenFileTable* ppoft = malloc(sizeof *ppoft);
	ppoft->pid = pid;
	ppoft->size = 0;

	for (int i = 0; i < MAX_OPEN_FILES_PER_PROCESS; i++) {
		ppoft->table[i] = NULL;
	}

	return ppoft;
}

int ppoft_add_data(PerProcessOpenFileTable* ppoft, PerProcessOpenFileData* ppofd) {
	int i = ppoft_find_free_entry(ppoft);

	if (i < 0)
		return -EMFILE;

	ppoft->table[i] = ppofd;
	ppoft->size++;

	return i;
}

int ppoft_find_free_entry(PerProcessOpenFileTable* ppoft) {
	for (int i = 0; i < MAX_OPEN_FILES_PER_PROCESS; i++) {
		if (!ppoft->table[i])
			return i;
	}

	return -1;
}