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

#include <qlist.h>
#include <qwidget.h>

extern bool shuttingDown;
extern bool checkingMail;

/** Contains and handles user identity information. */
class KMIdentity;
extern KMIdentity* identity;

/** Global animated busy pointer. */
class KBusyPtr;
extern KBusyPtr* kbp;

/** The application object. */
class KApplication;
extern KApplication* app;

/** Account manager: manages all accounts for mail retrieval. */
class KMAcctMgr;
extern KMAcctMgr* acctMgr;

/** Folder manager: manages all mail folders. */
class KMFolderMgr;
extern KMFolderMgr* folderMgr;

/** Sender: handles sending of messages. */
class KMSender;
extern KMSender* msgSender;

/** Global locale for localization of messages. */
//class KLocale;
//extern KLocale* nls;  // use i18n() now !

/** Standard accelerator keys. */
class KStdAccel;
extern KStdAccel* keys;

/** Filter manager: handles mail filters and applies them to messages. */
class KMFilterMgr;
extern KMFilterMgr* filterMgr;

/** Addressbook. */
class KMAddrBook;
extern KMAddrBook* addrBook;

/** Dictionary of available mail filter actions. */
class KMFilterActionDict;
extern KMFilterActionDict* filterActionDict;

/** A bunch of standard mail folders. */
class KMFolder;
extern KMFolder* inboxFolder;
extern KMFolder* outboxFolder;
extern KMFolder* sentFolder;
extern KMFolder* trashFolder;

/** The "about KMail" text. */
extern const char* aboutText;

void serverReady(bool flag);

#endif /*kmglobal_h*/
