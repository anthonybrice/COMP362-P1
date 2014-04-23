/**
* Name: Anthony Brice
* Lab/task: Project 1 Task 5
* Date: 04/23/14
**/

#ifndef _METADATANODE_H_
#define _METADATANODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>

#include "params.h"

/** The metadata node */
typedef struct {
	/** the name of the file */
	char name[MAX_NAME];

	/** the size of the file */
	size_t size;

	/** user id */
	int uid;

	/** group id */
	int gid;

	/** mode of the file */
	unsigned short fileMode;

	/** creation time */
	time_t ctime;

	/** modified time */
	time_t mtime;

	/** access time */
	time_t atime;

	/**
	* Number of hard links to file
	*
	* So far, this is always 1.
	*/
	unsigned int linkCount;

	/** index into storage of data or index node associated with file */
	StoragePointer dataIndex;
} MetadataNode;


/**
 * Display info about file
 *
 * Only useful for simulating a filesystem in C. Unused when run with fuse
 * interface.
 */
void displayFileInfo(MetadataNode* mdn);

/** Populate fields of a metadata node */
void fillMetadataNode(MetadataNode* mdn, const char* name, int uid, int gid, short fileMode, int dataIndex);

#endif