#include "MetaDataNode.h"

void fillMetaDataNode(MetaDataNode* mdn, const char* name, int uid, int gid, short fileMode, int dataIndex) {
	strcpy(mdn->name, name);

	mdn->size = 0;
	mdn->fileMode = fileMode;
	mdn->linkCount = 1;
	mdn->dataIndex = (StoragePointer) dataIndex;
	mdn->uid = uid;
	mdn->gid = gid;

	mdn->ctime = mdn->mtime = mdn->atime = time(NULL);
}

void displayFileInfo(MetaDataNode* mdn) {
	char buff[20];
	struct tm* timeInfo = localtime(&(mdn->mtime));
	strftime(buff, 20, "%b %d %H:%M", timeInfo);

	struct passwd* pwd = getpwuid(mdn->uid);
	struct group* grp = getgrgid(mdn->gid);

	printf("%o %d %s %s %d %s %s\n", mdn->fileMode, mdn->linkCount, pwd->pw_name, grp->gr_name, mdn->size, buff, mdn->name);
}

