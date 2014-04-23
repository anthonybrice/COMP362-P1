/**
* Name: Anthony Brice
* Lab/task: Project 1 Task 5
* Date: 04/23/14
**/

#ifndef _ABFS_H_
#define _ABFS_H_

#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <fuse.h>

#include "Filesystem.h"
#include "MetadataNode.h"
#include "Block.h"
#include "GlobalOpenFileTable.h"

#include "params.h"
#include "bitops.h"

/** The filesystem (defined in Filesystem.h) */
Filesystem* fs;

/** Remove a file */
static int abfs_unlink(const char* name);

/** Read directory
 *
 * I chose the first mode of operation described for readdir in fuse.h
 */
static int abfs_readdir(const char* name, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);

/** Get file attributes
 *
 * The function merely fills in the appropriate information from the file's
 * MetaDataNode into buf.
 */
static int abfs_getattr(const char* name, struct stat* buf);

/** Create and open a file
 *
 * If the file does not exist, create it with specified mode and open it. Copy
 * returned file descriptor to fi->fh. I was wrong earlier about fuse only
 * creating one fuse_file_info per file, so we may use fi->fh later.
 */
static int abfs_create(const char* name, mode_t mode, struct fuse_file_info* fi);

/** Check file access permissions */
static int abfs_access(const char* name, int mask);

/** File open operation
 *
 * Checks if the operation is permitted for the given flags and copies a file
 * descriptor to fi->fh to be used in subsequent calls to read(), write(),
 * and close().
 */
static int abfs_open(const char* name, struct fuse_file_info* fi);

/** Change the access and modification times of a file */
static int abfs_utimens(const char* name, const struct timespec ts[2]);

/** Change the size of a file */
static int abfs_truncate(const char* name, off_t size);

/** Read data from an open file
 *
 * Returns exactly the number of bytes requested except on EOF or error,
 * in which case the rest of the data is substituted with zeroes.
 */
static int abfs_read(const char* name, char* buf, size_t size, off_t offset, struct fuse_file_info* fi);

/** Write data to an open file
 *
 * Returns exactly the number of bytes requested except on error.
 */
static int abfs_write(const char* name, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi);

/** Change the permission bits of a file */
static int abfs_chmod(const char* name, mode_t mode);

/** Possibly flush cached data
 *
 * Called on each close() of a file descriptor.
 */
static int abfs_flush(const char* name, struct fuse_file_info* fi);

/** Release an open file.
 *
 * fuse isn't very clear about what exactly this does, but it says there
 * is one call to this for every open, so I check if a reference exists
 * for this file and pid in the file descriptors and remove them if so.
 */
static int abfs_release(const char* name, struct fuse_file_info* fi);

#endif

