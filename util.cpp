// Utility functions
#include <string.h>
#include <stdlib.h>
#include <qdir.h>
#include <qstrlist.h>
#include "util.h"

int windowCount;

char *getGecos()
{
	struct passwd *pwd = getpwnam(getlogin());

	/* Name is supposed to have no ',' */
	return strtok(pwd->pw_gecos, ",");
}

void removeDirectory(const char *path) {
	QDir dir;
	dir=path;
	if (!(dir.exists())) return;

	QStrList *list;
	char *s;
	unsigned int i;

	dir.setFilter(QDir::Files | QDir::Hidden);
	dir.setNameFilter("*");
	list=dir.entryList();
	for (i=0;i<list->count();i++) dir.remove(list->at(i));

	dir.setFilter(QDir::Dirs | QDir::Hidden);
	dir.setNameFilter("*");
	list=dir.entryList();
	for (i=0;i<list->count();i++) {
		s=list->at(i);
		if ((strcmp(s,".")!=0) && (strcmp(s,"..")!=0)) {
			dir.cd(s);
			removeDirectory(dir.path());
			dir.cdUp();
		}
	}

	dir.rmdir(dir.path());
}

