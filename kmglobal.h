/*  KMail Global Objects
 *
 *  These objects are all created in main.cpp during application start
 *  before anything else is done.
 *
 *  If you add anything here do not forget to define and create the
 *  object in main.cpp !
 *
 *  Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmglobal_h
#define kmglobal_h

extern bool shuttingDown;

class KMIdentity;
extern KMIdentity* identity;

class KBusyPtr;
extern KBusyPtr* kbp;

class KApplication;
extern KApplication* app;

class KMAcctMgr;
extern KMAcctMgr* acctMgr;

class KMFolderMgr;
extern KMFolderMgr* folderMgr;

class KMSender;
extern KMSender* msgSender;

class KLocale;
extern KLocale* nls;

class KShortCut;
extern KShortCut* keys;

class KMFolder;
class KMFolder;
extern KMFolder* inboxFolder;
extern KMFolder* outboxFolder;
extern KMFolder* sentFolder;
extern KMFolder* queuedFolder;
extern KMFolder* trashFolder;

#endif /*kmglobal_h*/
