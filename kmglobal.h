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

#endif /*kmglobal_h*/
