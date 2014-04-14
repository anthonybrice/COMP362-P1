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
		GList* l = fileSystem->directory[i];
		while (l) {
			printf("i == %d\n", i);
			struct stat st;
			MetaDataNode* mdn = &fileSystem->storage[GPOINTER_TO_UINT(l->data)].mdn;
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
		MetaDataNode* mdn = findFile(name, NULL, NULL);
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

	return fs_open(name, fi->flags, fc->uid, fc->gid, fc->pid);
}

static int abfs_access(const char* name, int mask) {
	struct fuse_context* fc = fuse_get_context();

	return fs_access(name, mask, fc->uid, fc->gid);
}

static int abfs_open(const char* name, struct fuse_file_info* fi) {
	struct fuse_context* fc = fuse_get_context();

	return fs_open(name, fi->flags, fc->uid, fc->gid, fc->pid);
}

static struct fuse_operations abfs_oper = {
	.getattr = abfs_getattr,
	.readdir = abfs_readdir,
	.create = abfs_create,
	.access = abfs_access,
	.unlink = abfs_unlink,
	.open = abfs_open,
	.destroy = freeFileSystem,
};

int main(int argc, char *argv[])
{
	newFileSystem();
	// printf("size mdn: %lud\n", sizeof (MetaDataNode));

	return fuse_main(argc, argv, &abfs_oper, NULL);
}