#ifndef _KMCONTROLIFACE
#define _KMCONTROLIFACE

#include <dcopobject.h>
#include <dcopref.h>
#include <kurl.h>

// checkMail won´t show reader but will check mail. use openReader to show
// if you give a filename to openReader it will show mbox or message
// if it is valid rfc-822 message or mbox file.
// You can pass hidden=1 to openComposer and it won´t be visible
// that way you can write messages and add attachments from other apps
// and send it via kmail. Should I add showAddressBook? hmm...
// The openComposer functions always return 1.
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
  virtual int openComposer(const QString &to, const QString &cc,
                           const QString &bcc, const QString &subject,
                           const QString &body, int hidden,
                           const KURL &messageFile,
			   const KURL &attachURL) = 0;
  virtual int openComposer (const QString &to, const QString &cc,
                            const QString &bcc, const QString &subject,
                            const QString &body, int hidden,
                            const QString &attachName,
                            const QCString &attachCte,
                            const QCString &attachData,
                            const QCString &attachType,
                            const QCString &attachSubType,
                            const QCString &attachParamAttr,
                            const QString &attachParamValue,
                            const QCString &attachContDisp) = 0;
  /** Open composer and return reference to DCOP interface of composer window.
    If hidden is true, the window will not be shown. If you use that option,
    it's your responsibility to call the send() function of the composer in
    order to actually send the mail. */
  virtual DCOPRef openComposer(const QString &to, const QString &cc,
                               const QString &bcc, const QString &subject,
                               const QString &body, bool hidden) = 0;

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

