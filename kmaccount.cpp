#include <stdlib.h>
#include <unistd.h>
#include <qdir.h>
#include <qstrlist.h>
#include "kmaccount.h"

KMAccount::KMAccount(const QString aname) {
	name=aname;

	QString s=QDir::homeDirPath();
	s+=("/.kmail/accounts/");
	s+=name;
	cfile=new QFile(s);
	cfile->open(IO_ReadWrite);
	cstream=new QTextStream(cfile);
	config=new KConfig(cstream);
}

KMAccount::~KMAccount() {
	config->sync();
	delete cfile;
	delete cstream;
	delete config;
}

void KMAccount::getLocation(QString *s) {
	QString t=config->readEntry("type");
	if (t=="inbox") {
		*s=config->readEntry("location");
	} else if (t=="pop3") {
		s->sprintf("{%s:%s/user=%s/service=pop3}",(const char *)config->readEntry("host"),(const char *)config->readEntry("port"),(const char *)config->readEntry("login"));
	} else {
		s->sprintf("{%s:%s/user=%s/service=imap}%s",(const char *)config->readEntry("host"),(const char *)config->readEntry("port"),(const char *)config->readEntry("mailbox"),(const char *)config->readEntry("login"));
	}
}

void KMAccount::initAuth(const QString login,const QString password) {
	config->writeEntry("login",login);
	config->writeEntry("password",password);
}

KMAccountMan::KMAccountMan() {
	setAutoDelete(TRUE);

	refreshList();
}

void KMAccountMan::refreshList() {
	QDir dir;
	QStrList *list;
	unsigned int i;
	char *s;

	clear();

	dir.cd(QDir::homeDirPath());
	dir.cd(".kmail/accounts");
	dir.setFilter(QDir::Files | QDir::Hidden);
	dir.setNameFilter("*");
	list=dir.entryList();
	for (i=0;i<list->count();i++) {
		s=list->at(i);
		append(new KMAccount(s));
	}
}

KMAccount *KMAccountMan::createAccount(const QString name) {
	KMAccount *a;

	a=new KMAccount(name);
	append(a);
	return a;
}

KMAccount *KMAccountMan::findAccount(const QString name) {
	for (unsigned int i=0;i<count();i++)
	  if (at(i)->name==name) return at(i);
	return NULL;
}

int KMAccountMan::removeAccount(const QString name) {
	KMAccount *a;
	QDir dir;

	if ((a=findAccount(name))==NULL) return 0;
	dir.cd(QDir::homeDirPath());
	dir.cd(".kmail/accounts");
	dir.remove(name);
	remove(a);
	return 1;
}

int KMAccountMan::renameAccount(const QString src,const QString dest) {
	QDir dir;
	clear();
	dir.cd(QDir::homeDirPath());
	dir.cd(".kmail/accounts");
	dir.rename(src,dest,FALSE);
	refreshList();
	return 1;
}

