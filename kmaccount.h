#ifndef __KMACCOUNT
#define __KMACCOUNT

#include <qfile.h>
#include <qstring.h>
#include <qtstream.h>
#include <Kconfig.h>
#include "mclass.h"

class KMAccount {
public:
	KMAccount(const QString aname);
	~KMAccount();
	void getLocation(QString *s);  // fills s with the location string
	void initAuth(const QString login,const QString password);

	KConfig *config;
	QFile *cfile;
	QTextStream *cstream;
	QString name;
};

typedef QList<KMAccount> KMAccountManBase;

class KMAccountMan : public KMAccountManBase {
public:
	KMAccountMan();

	void refreshList();
	KMAccount *createAccount(const QString name);
	KMAccount *findAccount(const QString name);
	int removeAccount(const QString name);
	int renameAccount(const QString src,const QString dest);
};

#endif

