#include <stdlib.h>
#include <qfileinf.h>
#include "util.h"
#include "kmfoldertree.moc"

KMFolderTree::KMFolderTree(QWidget *parent=0,const char *name=0) : KTreeList(parent,name) {
	connect(this,SIGNAL(highlighted(int)),this,SLOT(doFolderSelected(int)));
	getList();
}

void KMFolderTree::cdFolder(QDir *dir,int index=-1) {
	KPath p,q;
	QString *s;
	dir->cd(QDir::home().path());
	dir->cd(".kmail/folders");
	if (index==-1) {
		index=currentItem();
		if (index==-1) return;
	}
	p=*(itemPath(index));
	while (!(p.isEmpty())) {
		s=p.pop();      	// reverse the path order, so that I can cd with it
		q.push(s);		// the treewidget needs a redesign :-(
	}
	q.pop();			// get rid of "/"
	while (!(q.isEmpty())) dir->cd(*q.pop());
}

void KMFolderTree::getList() {
	QDir *d;
	KPath *p;
	char buf[255];
	strcpy(buf,QDir::home().path());
	strcat(buf,"/.kmail/folders");
	clear();
	insertItem("/",NULL,-1);
	d=new QDir(buf,NULL,QDir::Name,QDir::Dirs);
	p=new KPath();
	p->push(new QString("/"));
	getListRecur(d,p);
	delete p;
	delete d;
	setExpandLevel(10);
}

void KMFolderTree::getListRecur(QDir *d,KPath *p) {
	QString s;
	d->setFilter(QDir::Dirs | QDir::Hidden);
	d->setNameFilter("*");
	const QFileInfoList *list=d->entryInfoList();
	QFileInfoListIterator it(*list);
	QFileInfo *fi;
	while ((fi=(it.current()))!=0) {
		if ((fi->fileName()!=".") && (fi->fileName()!="..")) {
			s=fi->fileName();
			addChildItem(s,NULL,p);
			QDir *e=new QDir(*d);
			e->cd(s);
			p->push(&s);
			getListRecur(e,p);
			p->pop();
			delete e;
		}
		++it;
	}
}

void KMFolderTree::doFolderSelected(int index) {
	QDir d;
	cdFolder(&d,index);
	d.convertToAbs();
	emit folderSelected(&d);
}

