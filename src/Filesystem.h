/**
* Name: Anthony Brice
* Lab/task: Project 1 Task 5
* Date: 04/23/14
**/

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <fcntl.h>
#include <errno.h>

#include "MetadataNode.h"
#include "Block.h"
#include "GlobalOpenFileTable.h"

#include "params.h"
#include "bitops.h"

#define MAX_SIZE STORAGE_SIZE * DATA_SIZE


/** The filesystem */
typedef struct {
	/**
	 * The flat directory structure
	 *
	 * An array of linked lists whose values are indices into storage of
	 * MetadataNodes. Indices into the directory are given by an internal
	 * hash function that operates on the name of a file. Collisions are
	 * handled by prepending to the linked list at the given index.
	 */
	GList* directory[DIRECTORY_SIZE];

	/**
	 * The filesystem's storage
	 *
	 * An array of Blocks. Blocks can be either a Metadata node, an index node,
	 * or a data node.
	 */
	Block storage[STORAGE_SIZE];

	/**
	 * A bitvector corresponding to blocks in storage
	 *
	 * 1 means open. 0 means in use.
	 */
	Byte freeSpace[BITNSLOTS(STORAGE_SIZE)];

	/**
	 * The global open file table
	 *
	 * One entry exists for each open file.
	 */
	GList* goft;

	/**
	 * Linked list of per process open file tables
	 *
	 * One entry exists for each process with an open file.
	 */
	GList* processList;
} Filesystem;

extern Filesystem* fs;

/** Initialize fs */
void fs_init();

/**
 * Create a new file
 *
 * Sets two blocks in storage, one for the metadata node and one for the
 * data node. Fills the metadata node with the mode and owner info. Puts
 * the index of the metadata node in the directory structure.
 */
int fs_create(const char* name, int mode, int uid, int gid);

/**
 * Remove a file
 *
 * Clears the appropriate entries in the bitvector.
 */
int fs_unlink(const char* name, int uid, int gid);

/**
 * Open a file
 *
 * Checks if the operation is permitted and if so returns a file descriptor
 * for a new entry in the file tables.
 */
int fs_open(const char* name, int flags, int uid, int gid, int pid);

/** Check access permissions */
int fs_access(const char* name, int amode, int uid, int gid);

/**
 * Close an open file
 *
 * Decrements the entry for the file in the global open file table and
 * removes the entry from the appropriate per process open file table.
 * If the size of either is 0, than that table is removed from the
 * respective list.
 */
int fs_release(const char* name, int fd, int pid);

/** Read from an open file */
int fs_read(int fd, char* buf, size_t size, int pid);

/** Write to an open file */
int fs_write(int fd, const char* buf, size_t size, int pid);

/**
 * Find the file table for the given pid
 *
 * Used internally, but is also available for use by the fuse interface
 * to move the offset of the position pointer in the per process open file
 * data.
 */
PerProcessOpenFileTable* fs_find_ppoft(int pid);

/** Change access and modify times */
int fs_utimens(const char* name, const struct timespec ts[2]);

/** Change the size of a file */
int fs_truncate(const char* name, off_t size);

/** Change the permissions of a file */
int fs_chmod(const char* name, mode_t mode, int uid);

/**
 * Find a file.
 *
 * Returns a pointer the metadata node for the file given by name. hashNum
 * and stIndex are available in case the caller wants respectively the index
 * into the directory structure, or the index into storage. Used internally
 * but is also available for use by the fuse interface.
 */
MetadataNode* find_file(const char* name, unsigned long* hashNum, StoragePointer* stIndex);

/** Free fs */
void fs_free();

#endif