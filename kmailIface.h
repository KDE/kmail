#ifndef _KMCONTROLIFACE
#define _KMCONTROLIFACE

#include <dcopobject.h>
#include <kurl.h>

// checkMail won´t show reader but will check mail. use openReader to show
// if you give a filename to openReader it will show mbox or message
// if it is valid rfc-822 message or mbox file.
// You can pass hidden=1 to openComposer and it won´t be visible
// that way you can write messages and add attachments from other apps
// and send it via kmail. Should I add showAddressBook? hmm...
// openComposer returns id of composer
// that id must be passed to setBody, addAttachment and send
// I´m not sure is ready() needed.
// sven <radej@kde.org>
class KMailIface : virtual public DCOPObject
{
  K_DCOP

k_dcop:
  virtual void checkMail() = 0;
  virtual void openReader() = 0;
  virtual int openComposer(const QString &to, const QString &cc,
                           const QString &bcc, const QString &subject,
                           const QString &body, int hidden,
                           const KURL &messageFile) = 0;
  virtual int send(int composerId, int how) = 0; //0=default,1=now,2=later
  virtual int addAttachment(int composerId, KURL url,
                            QString comment) = 0;
  virtual int setBody (int composerId, QString body) = 0;
  virtual int ready() = 0; //1=yes, 0=no
  virtual void compactAllFolders() = 0;

//  pre : foldername : the requested foldername in kmail (at the
//                     zero level in the foldertree.
//        messagefile: the name of the filename (local) with the
//                     message to be added.
//  post: =1,  message added to folder, if folder doesn't exist, folder
//             has been created.
//        =0,  an error occured.
//        =-1, couldn't create folder and it didn't exist
//        =-2, couldn't read messageFile.
//        =-3, Can't allocate memory.
//        =-4, Message already exists in folder.
  virtual int dcopAddMessage(const QString & foldername,
                             const QString & messagefile) = 0;
  virtual int dcopAddMessage(const QString & foldername,
                             const KURL & messagefile) = 0;
};

#endif

