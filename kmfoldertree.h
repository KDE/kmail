#ifndef __KMFOLDERTREE
#define __KMFOLDERTREE

#include <qwidget.h>
#include <qdir.h>
#include <ktreelist.h>

class KMFolderTree : public KTreeList {
		Q_OBJECT
	public:
		KMFolderTree(QWidget *parent=0,const char *name=0);
		void cdFolder(QDir *dir,int index=-1);	// cd's "dir" to the directory of folder "index"
		void getList();				// get/refresh the folder tree
	private:
		void getListRecur(QDir *,KPath *);
	signals:
		void folderSelected(QDir *);		// path to folder
	private slots:
		void doFolderSelected(int);
};

#endif

