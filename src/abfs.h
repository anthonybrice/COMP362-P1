#ifndef _ABFS_H_
#define _ABFS_H_

#define FUSE_USE_VERSION 29

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

Filesystem* fs;

static int abfs_unlink(const char* name);
static int abfs_readdir(const char* name, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
static int abfs_getattr(const char* name, struct stat* buf);
static int abfs_create(const char* name, mode_t mode, struct fuse_file_info* ffi);
static int abfs_access(const char* name, int mask);
static int abfs_open(const char* name, struct fuse_file_info* fi);

#endif

