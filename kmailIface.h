#ifndef _KMCONTROLIFACE
#define _KMCONTROLIFACE

// no forward declarations - dcopidl2cpp won't work
#include <dcopobject.h>
#include <dcopref.h>
#include <kurl.h>
#include <qstringlist.h>

/** checkMail won´t show reader but will check mail. use openReader to
    show if you give a filename to openReader it will show mbox or
    message if it is valid rfc-822 message or mbox file.  You can pass
    hidden=1 to openComposer and it won´t be visible that way you can
    write messages and add attachments from other apps and send it via
    kmail. Should I add showAddressBook? hmm...  The openComposer
    functions always return 1.  sven <radej@kde.org> */
class KMailIface : virtual public DCOPObject
{
  K_DCOP

k_dcop:
  virtual void checkMail() = 0;
  virtual QStringList accounts() = 0;
  virtual void checkAccount(const QString &account) = 0;
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
  virtual int openComposer(const QString &to, const QString &cc,
                           const QString &bcc, const QString &subject,
                           const QString &body, int hidden,
                           const KURL &messageFile,
                           const KURL::List &attachURLs) = 0;
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
                            const QCString &attachContDisp,
                            const QCString &attachCharset) = 0;
  /** Open composer and return reference to DCOP interface of composer window.
    If hidden is true, the window will not be shown. If you use that option,
    it's your responsibility to call the send() function of the composer in
    order to actually send the mail. */
  virtual DCOPRef openComposer(const QString &to, const QString &cc,
                               const QString &bcc, const QString &subject,
                               const QString &body, bool hidden) = 0;

    /**
       Send a certificate request to the CA specified in \a to. The
       certificate is stored in the byte array \a certData. It needs
       to stored according to BER and PKCS#10.
       This method will set content type encoding, mime types, etc. as
       per the MailTT specification.
    */
    virtual int sendCertificate( const QString& to,
                                 const QByteArray& certData ) = 0;


  virtual void compactAllFolders() = 0;

  /** @param foldername the requested foldername in kmail (at the
                     zero level in the foldertree.
      @param messagefile: the name of the filename (local) with the
                     message to be added.
  @return =1,  message added to folder, if folder doesn't exist, folder
             has been created.
        =0,  an error occurred.
        =-1, couldn't create folder and it didn't exist
        =-2, couldn't read messageFile.
        =-3, Can't allocate memory.
        =-4, Message already exists in folder.
  */
  virtual int dcopAddMessage(const QString & foldername,
                             const QString & messagefile) = 0;
  virtual int dcopAddMessage(const QString & foldername,
                             const KURL & messagefile) = 0;

  virtual QStringList folderList() const =0;
  virtual DCOPRef getFolder( const QString& vpath ) =0;
  virtual void selectFolder( QString folder ) =0;
  virtual bool canQueryClose() =0;

  virtual int timeOfLastMessageCountChange() const =0;

k_dcop_signals:
  void unreadCountChanged();

  void unreadCountChanged( const QString& folderURL, int numUnread );

k_dcop_hidden:
  /** DCOP call which is used by the Kontact plugin to create a new message. */
  virtual DCOPRef newMessage() = 0;

  virtual bool showMail( Q_UINT32 serialNumber, QString messageId ) = 0;
  /**
   * DCOP-enabled for KMailUniqueAppHandler in the kontact plugin
   * @param noArgsOpensReader true in the kmail process, meaning that launching "kmail"
   * will open a reader window or bring to front an existing one.
   * noArgsOpensReader is false when this is called from kontact, so that typing
   * "kmail" doesn't open a window.
   * Returns true if the command line was handled, false if it was empty and
   * not handled (due to noArgsOpensReader==false).
   */
  virtual bool handleCommandLine( bool noArgsOpensReader ) = 0;
  /**
   *
   * DCOP-enabled for use in kaddressbook drop
   */
  virtual QString getFrom( Q_UINT32 serialNumber ) = 0;
};

#endif
