/**
* Name: Anthony Brice
* Lab/task: Project 1 Task 5
* Date: 04/23/14
**/

#ifndef _GLOBALOPENFILETABLE_H_
#define _GLOBALOPENFILETABLE_H_

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "MetadataNode.h"
#include "Block.h"

#include "params.h"

/** Data of an entry in the global open file table */
typedef struct {
	char name[MAX_NAME];
	MetadataNode* mdn;
	unsigned int fileOpenCount;
} GlobalOpenFileData;

/**
 * Data of an entry in a per process open file table
 *
 * index * DATA_SIZE + position is the number of bytes into the file
 * that the entry points to.
 */
typedef struct {
	short index;
	short position;

	/** open flags */
	int flags;
	GlobalOpenFileData* gofd;
} PerProcessOpenFileData;

/** a per process open file table */
typedef struct {
	/**
	 * An array of pointers to per process open file data
	 *
	 * An entry exists for each call to open().
	 */
	PerProcessOpenFileData* table[MAX_OPEN_FILES_PER_PROCESS];

	/** number of entries in the table */
	int size;

	/** pid associated with this table */
	int pid;
} PerProcessOpenFileTable;

PerProcessOpenFileData* ppofd_init(int uid, int gid, GlobalOpenFileData* gofd, int flags);
PerProcessOpenFileTable* ppoft_init(int pid);
GlobalOpenFileData* gofd_init(MetadataNode* mdn);
PerProcessOpenFileTable* ppoft_init(int pid);

/** find an entry in the global open file table */
GlobalOpenFileData* goft_find_by_name(GList*, const char* name);

/** find the index of the first free entry in a per process open file table */
int ppoft_find_free_entry(PerProcessOpenFileTable* ppoft);

/**
 * Change the number of bytes to which the entry points to the number of
 * bytes given by offset
 *
 * Used by the fuse interface in calls to read() and write(). It seems that
 * the given offset in those calls is not always where the entry in the
 * open file tables pointed, but I'm not sure why that is the case.
 */
void ppofd_move_offset(PerProcessOpenFileData* ppofd, off_t offset);

/** Add an entry to a per process open file table */
int ppoft_add_data(PerProcessOpenFileTable* ppoft, PerProcessOpenFileData* ppofd);


#endif