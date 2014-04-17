#include "abfs.h"

static int abfs_unlink(const char* name) {
	struct fuse_context* fc = fuse_get_context();

	return fs_unlink(name, fc->uid, fc->gid);
}

static int abfs_readdir(const char* name, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	if(strcmp(name, "/"))
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (int i = 0; i < DIRECTORY_SIZE; i++) {
		GList* l = fs->directory[i];
		while (l) {
			printf("i == %d\n", i);
			struct stat st;
			MetadataNode* mdn = &fs->storage[GPOINTER_TO_UINT(l->data)].mdn;
			abfs_getattr(mdn->name, &st);
			/* call to fs_access?*/
			if (filler(buf, mdn->name + 1, &st, 0))
				return -ENOMEM;
			l = l->next;
		}
	}

	return 0;
}

static int abfs_getattr(const char* name, struct stat* buf) {
	memset(buf, 0, sizeof *buf);

	if (!strcmp(name, "/")) {
		buf->st_mode = S_IFDIR | 0755;
		buf->st_nlink = 2;
	} else {
		MetadataNode* mdn = find_file(name, NULL, NULL);
		if (!mdn)
			return -ENOENT;

		buf->st_mode = mdn->fileMode;
		buf->st_nlink = mdn->linkCount;
		buf->st_uid = mdn->uid;
		buf->st_gid = mdn->gid;
		buf->st_size = mdn->size;
		buf->st_atime = mdn->atime;
		buf->st_mtime = mdn->mtime;
		buf->st_ctime = mdn->ctime;
	}

	return 0;
}

static int abfs_create(const char* name, mode_t mode, struct fuse_file_info* fi) {
	struct fuse_context* fc = fuse_get_context();

	int i = fs_create(name, mode, fc->uid, fc->gid);
	if (i < 0)
		return i;

	return 0;
}

static int abfs_access(const char* name, int mask) {
	struct fuse_context* fc = fuse_get_context();

	return fs_access(name, mask, fc->uid, fc->gid);
}

static int abfs_open(const char* name, struct fuse_file_info* fi) {
	struct fuse_context* fc = fuse_get_context();

	int fd = fs_open(name, fi->flags, fc->uid, fc->gid, fc->pid);
	if (fd < 0)
		return fd;

	fs_release(name, fd, fc->pid);

	return 0;
}

static int abfs_read(const char* name, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	struct fuse_context* fc = fuse_get_context();

	// int flag = O_RDONLY;
	int fd = fs_open(name, O_RDONLY, fc->uid, fc->gid, fc->pid);

	if (fd < 0) {
		fs_release(name, fd, fc->pid);
		return fd;
	}

	PerProcessOpenFileData* ppofd = fs_find_ppoft(fc->pid)->table[fd];
	ppofd_move_offset(ppofd, offset);

	int ret = fs_read(fd, buf, size, fc->pid);

	fs_release(name, fd, fc->pid);

	return ret;
}

static int abfs_write(const char* name, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	struct fuse_context* fc = fuse_get_context();

	// int flag = O_WRONLY;
	int fd = fs_open(name, O_WRONLY, fc->uid, fc->gid, fc->pid);

	if (fd < 0) {
		fs_release(name, fd, fc->pid);
		return fd;
	}

	PerProcessOpenFileData* ppofd = fs_find_ppoft(fc->pid)->table[fd];
	ppofd_move_offset(ppofd, offset);

	int ret = fs_write(fd, buf, size, fc->pid);

	fs_release(name, fd, fc->pid);

	return ret;
}

static int abfs_release(const char* name, struct fuse_file_info* fi) {
	// struct fuse_context* fc = fuse_get_context();

	return 0;
}

static struct fuse_operations abfs_oper = {
	.getattr = abfs_getattr,
	.readdir = abfs_readdir,
	.create = abfs_create,
	.access = abfs_access,
	.unlink = abfs_unlink,
	.open = abfs_open,
	.destroy = freeFilesystem,
	.release = abfs_release,
	.read = abfs_read,
	.write = abfs_write,
};

int main(int argc, char *argv[])
{
	fs_init();
	// printf("size mdn: %lu\n", sizeof (MetadataNode));

	// Block block;
	// printf("size Block: %lu\n", sizeof block);
	// printf("size data: %lu\n", sizeof block.data);
	// printf("size blocktype: %lu\n", sizeof block.type);

	// Block b;
	// printf("size indexNode: %lu\n", sizeof b.indexNode);

	return fuse_main(argc, argv, &abfs_oper, NULL);
}
