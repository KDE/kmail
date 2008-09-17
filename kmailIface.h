#ifndef _KMCONTROLIFACE
#define _KMCONTROLIFACE

// no forward declarations - dcopidl2cpp won't work
#include <dcopobject.h>
#include <dcopref.h>
#include <kurl.h>
#include <qstringlist.h>

/** checkMail wont show reader but will check mail. use openReader to
    show if you give a filename to openReader it will show mbox or
    message if it is valid rfc-822 message or mbox file.  You can pass
    hidden=1 to openComposer and it wont be visible that way you can
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
                            const QCString &attachCharset,
                            uint identity) = 0;
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
      @param messagefile the name of the filename (local) with the
                     message to be added.
      @param MsgStatusFlags a string coding the status of the message
             with a char for each status e.g. a 'N' for new
             this param is optional
      @return =1,  message added to folder, if folder doesn't exist, folder
             has been created.
        =0,  an error occurred.
        =-1, couldn't create folder and it didn't exist
        =-2, couldn't read messageFile.
        =-3, Can't allocate memory.
        =-4, Message already exists in folder.
  */
  virtual int dcopAddMessage(const QString & foldername,
                             const QString & messagefile,
                             const QString & MsgStatusFlags = QString()) = 0;
  virtual int dcopAddMessage(const QString & foldername,
                             const KURL & messagefile,
                             const QString & MsgStatusFlags = QString()) = 0;

  virtual QStringList folderList() const =0;
  virtual DCOPRef getFolder( const QString& vpath ) =0;
  virtual void selectFolder( QString folder ) =0;
  virtual bool canQueryClose() =0;

  /**
   * Set the KMail Default transport.
   * @param transport the name of the transport as defined in the sending
            accounts configuration.
  */
  virtual void setDefaultTransport( const QString & transport ) =0;

  virtual int timeOfLastMessageCountChange() const =0;

  /**
   * Abort any running compaction/expiry, and don't launch any new ones
   * until resumeBackgroundJobs() is called
   */
  virtual void pauseBackgroundJobs() = 0;

  /**
   * Resume compaction/expiry, and allow launching new ones
   */
  virtual void resumeBackgroundJobs() = 0;

  /**
   * Stop all network related jobs and enter offline mode
  */
  virtual void stopNetworkJobs() = 0;

  /**
   * Resume all network related jobs and enter online mode
  */
  virtual void resumeNetworkJobs() = 0;

k_dcop_signals:
  void unreadCountChanged();

  void unreadCountChanged( const QString& folderURL, int numUnread );

k_dcop_hidden:
  /** DCOP call which is used by the Kontact plugin to create a new message.
   *
   *  @TODO Rename to newMessageInternal()
   *  @TODO Merge this and the various openComposer methods for better code reuse
  */
  virtual DCOPRef newMessage(const QString &to,
                             const QString &cc,
                             const QString& bcc,
                             bool hidden,
                             bool useFolderId,
                             const KURL &messageFile,
                             const KURL &attachURL) = 0;

  /** Shows the specified message in a separate message window.
      @param serialNumber the serial number of the message to be shown.
      @param messageId this parameter is ignored.
  */
  /*  @TODO Get rid of the messageId parameter.  */
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
  virtual bool firstStart() = 0;
  /**
   *
   * DCOP-enabled for use in kaddressbook drop
   */
  virtual QString getFrom( Q_UINT32 serialNumber ) = 0;
  virtual QString debugScheduler() = 0;
  virtual QString debugSernum( Q_UINT32 serialNumber ) = 0;

  /** Does essentially the same as dcopAddMessage except that it doesn't reject
      duplicate messages.

      @param foldername the requested foldername in kmail (at the
                     zero level in the foldertree.
      @param messagefile: the name of the filename (local) with the
                     message to be added.
      @param MsgStatusFlags a string coding the status of the message
                     with a char for each status e.g. a 'N' for new
                     this param is optional
      @return =1,  message added to folder, if folder doesn't exist, folder
             has been created.
        =0,  an error occurred.
        =-1, couldn't create folder and it didn't exist
        =-2, couldn't read messageFile.
        =-3, Can't allocate memory.
        =-4, Message already exists in folder.
  */
  virtual int dcopAddMessage_fastImport(const QString & foldername,
                                        const QString & messagefile,
                                        const QString & MsgStatusFlags = QString()) = 0;
  virtual int dcopAddMessage_fastImport(const QString & foldername,
                                        const KURL & messagefile,
                                        const QString & MsgStatusFlags = QString()) = 0;

  /** Clears the list of added message ids which is used to filter out
      duplicates. */
  virtual void dcopResetAddMessage() = 0;

  virtual void loadProfile( const QString& path ) = 0;
  virtual void saveToProfile( const QString& path ) const = 0;
};

#endif
