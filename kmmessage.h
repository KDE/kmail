/* kmmessage.h: Mime Message Class
 *
 */
#ifndef kmmessage_h
#define kmmessage_h

#include <mimelib/string.h>
#include "kmmsgbase.h"

class QStringList;
class QString;
class QTextCodec;
class QStrList;

class KMFolder;
class DwMessage;
class KMMessagePart;
class KMMsgInfo;
class KMHeaders;

class DwBodyPart;
class DwMediaType;
class DwHeaders;

#define KMMessageInherited KMMsgBase
class KMMessage: public KMMsgBase
{
  friend class KMFolder;
  friend class KMHeaders;    // needed for MIME Digest forward

public:
  /** Straight forward initialization. */
  KMMessage(KMFolder* parent=NULL);

  /** Constructor from a DwMessage. */
  KMMessage(DwMessage*);

  /** Copy constructor. Does *not* automatically load the message. */
  KMMessage(KMMsgInfo& msgInfo);

  /** Copy constructor */
  KMMessage( const KMMessage& other );

  /** assignment operator */
  const KMMessage& operator=( const KMMessage& other ) {
    if( &other == this )
      return *this;
    assign( other ); return *this;
  }

  /** Destructor. */
  virtual ~KMMessage();

  /** Returns TRUE if object is a real message (not KMMsgInfo or KMMsgBase) */
  virtual bool isMessage(void) const;

  /** Mark the message as deleted */
  void del(void) { setStatus(KMMsgStatusDeleted); }

  /** Undelete the message. Same as touch */
  void undel(void) { setStatus(KMMsgStatusOld); }

  /** Touch the message - mark it as read */
  void touch(void) { setStatus(KMMsgStatusOld); }

  /** Create a new message that is a reply to this message, filling all
      required header fields with the proper values. The returned message
      is not stored in any folder. Marks this message as replied. */
  virtual KMMessage* createReply(bool replyToAll=FALSE, bool replyToList=FALSE,
				 QString selection=QString::null, bool noQuote=FALSE,
				 bool allowDecryption=TRUE);

  /** Create a new message that is a redirect to this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as replied.
    Redirects differ from forwards so they are forwarded to some other
    user, mail is not changed and the reply-to field is set to
    the email adress of the original sender
   */
  virtual KMMessage* createRedirect();

  /** Create a new message that is a "failed delivery" reply to this
    message, filling all required header fields with the proper
    values. The returned message is not stored in any folder. If @p
    withUI is true, asks the user if he really wants that. */
  virtual KMMessage* createBounce( bool withUI );

  /** Create the forwarded body for the message. */
  virtual QCString createForwardBody(void);

  /** Create a new message that is a forward of this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as forwarded. */
  virtual KMMessage* createForward(void);

  /** Create a new message that is a delivery receipt of this message,
      filling required header fileds with the proper values. The
      returned message is not stored in any folder. */
  virtual KMMessage* createDeliveryReceipt(void) const;

  /** Parse the string and create this message from it. */
  virtual void fromString(const QCString& str, bool setStatus=FALSE);

  /** Return the entire message contents in the DwString. This function
      is *fast* even for large message since it does *not* involve a
      string copy.
  */
  virtual const DwString& asDwString(void);

  /** Return the entire message contents as a string. This function is
      slow for large message since it involves a string copy. If you
      need the string representation only for a short time
      (i.e. without the chance of calling any function in the
      underlying mimelib, then you should use the @ref asByteArray,
      which is more efficient or use the @ref asDwString function.
      @see asByteArray
      @see asDwString
  */
  virtual QCString asString(void);

  /**
   * Return the message contents besides the headers that should not be sent.
   */
  virtual QCString asSendableString();

  /** Return reference to Content-Type header for direct manipulation. */
  DwMediaType& dwContentType(void);

  /** Return header as string. */
  virtual QString headerAsString(void) const;

  /** Returns message body with quoting header and indented by the
    given indentation string. This is suitable for including the message
    in another message of for replies, forwards. The header string is
    a template where the following fields are replaced with the
    corresponding values:
    <pre>
	%D: date of this message
	%S: subject of this message
	%F: sender (from) of this message
	%%: a single percent sign
    </pre>
    No attachments are handled if includeAttach is false.
    The signature is stripped if aStripSignature is true and
    smart quoting is turned on. Signed or encrypted texts
    get converted to plain text when allowDecryption is true. */
  virtual QCString asQuotedString(const QString& headerStr,
                                  const QString& indentStr,
                                  const QString& selection=QString::null,
                                  bool aStripSignature=true,
                                  bool allowDecryption=true) const;

  /** Initialize header fields. Should be called on new messages
    if they are not set manually. E.g. before composing. Calling
    of setAutomaticFields(), see below, is still required. */
  virtual void initHeader(const QString& id=QString::fromLatin1("unknown"));

  /** Initialize headers fields according to the identity and the transport
    header of the given original message */
  virtual void initFromMessage(const KMMessage *msg, bool idHeaders = TRUE);

  /** Removes empty fields from the header, e.g. an empty Cc: or Bcc:
    field. */
  virtual void cleanupHeader(void);

  /** Set fields that are either automatically set (Message-id)
    or that do not change from one message to another (MIME-Version).
    Call this method before sending *after* all changes to the message
    are done because this method does things different if there are
    attachments / multiple body parts. */
  virtual void setAutomaticFields(bool isMultipart=FALSE);

  /** Get or set the 'Date' header field */
  virtual QString dateStr(void) const;
  virtual QCString dateShortStr(void) const;
  virtual QString dateIsoStr(void) const;
  virtual time_t date(void) const;
  virtual void setDate(const QCString& str);
  virtual void setDate(time_t aUnixTime);

  /** Set the 'Date' header field to the current date. */
  virtual void setDateToday(void);

  /** Get or set the 'To' header field */
  virtual QString to(void) const;
  virtual void setTo(const QString& aStr);
  virtual QString toStrip(void) const;

  /** Get or set the 'ReplyTo' header field */
  virtual QString replyTo(void) const;
  virtual void setReplyTo(const QString& aStr);
  virtual void setReplyTo(KMMessage*);

  /** Get or set the 'Cc' header field */
  virtual QString cc(void) const;
  virtual void setCc(const QString& aStr);

  /** Get or set the 'Bcc' header field */
  virtual QString bcc(void) const;
  virtual void setBcc(const QString& aStr);

  /** Get or set the 'Fcc' header field */
  virtual QString fcc(void) const;
  virtual void setFcc(const QString& aStr);

  /** Get or set the 'Drafts' folder */
  virtual QString drafts(void) const { return mDrafts; }
  virtual void setDrafts(const QString& aStr);

  /** Get or set the 'From' header field */
  virtual QString from(void) const;
  virtual void setFrom(const QString& aStr);
  virtual QString fromStrip(void) const;
  virtual QCString fromEmail(void) const;

  /** Get or set the 'Who' header field. The actual field that is
      returned depends on the contents of the owning folders whoField().
      Usually this is 'From', but it can also contain 'To'. */
  virtual QString who(void) const;

  /** Get or set the 'Subject' header field */
  virtual QString subject(void) const;
  virtual void setSubject(const QString& aStr);

  /** Get or set the 'X-Mark' header field */
  virtual QString xmark(void) const;
  virtual void setXMark(const QString& aStr);

  /** Get or set the 'In-Reply-To' header field */
  virtual QString replyToId(void) const;
  virtual void setReplyToId(const QString& aStr);
  virtual QString replyToIdMD5(void) const;

  /** Get or set the 'Message-Id' header field */
  virtual QString msgId(void) const;
  virtual void setMsgId(const QString& aStr);
  virtual QString msgIdMD5(void) const;

  /** Set the references for this message */
  virtual void setReferences(const QCString& aStr);

  /** Returns the message ID, useful for followups */
  virtual QCString id(void) const;

  /** Returns the message serial number. */
  virtual unsigned long getMsgSerNum() const;
  /** Sets the message serial number.  If defaulted to zero, the
    serial number will be assigned using the dictionary. */
  virtual void setMsgSerNum(unsigned long newMsgSerNum = 0);

  /** Get or set header field with given name */
  virtual QString headerField(const QCString& name) const;
  virtual void setHeaderField(const QCString& name, const QString& value);

  /** Returns header address list as string list. Warning: returns
      a temporary object !
      Valid for the following fields: To, Bcc, Cc, ReplyTo, ResentBcc,
      ResentCc, ResentReplyTo, ResentTo */
  virtual QStrList headerAddrField(const QCString& name) const;

  /** Remove header field with given name */
  virtual void removeHeaderField(const QCString& name);

  /** Get or set the 'Content-Type' header field
      The member functions that involve enumerated types (ints)
      will work only for well-known types or subtypes. */
  virtual QCString typeStr(void) const;
  virtual int type(void) const;
  virtual void setTypeStr(const QCString& aStr);
  virtual void setType(int aType);
  /** Subtype */
  virtual QCString subtypeStr(void) const;
  virtual int subtype(void) const;
  virtual void setSubtypeStr(const QCString& aStr);
  virtual void setSubtype(int aSubtype);
  /** add or change a parameter of a DwMediaType field */
  static void setDwMediaTypeParam( DwMediaType &mType,
                                   const QCString& attr,
                                   const QCString& val );
  /** add or change a parameter of the Content-Type field */
  virtual void setContentTypeParam(const QCString& attr, const QCString& val);

  /** get the DwHeaders */
  virtual DwHeaders& headers(void);

  /** Get or set the 'Content-Transfer-Encoding' header field
      The member functions that involve enumerated types (ints)
      will work only for well-known encodings. */
  virtual QCString contentTransferEncodingStr(void) const;
  virtual int  contentTransferEncoding(void) const;
  virtual void setContentTransferEncodingStr(const QCString& aStr);
  virtual void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  QCString cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const QCString& aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }

  /** Get the message body. Does not decode the body. */
  virtual QCString body(void) const;

  /** Set the message body. Does not encode the body. */
  virtual void setBody(const QCString& aStr);

  /** Set the message body, encoding it according to the current content
      transfer encoding. The first method for null terminated strings,
      the second for binary data */
  virtual void setBodyEncoded(const QCString& aStr);
  virtual void setBodyEncodedBinary(const QByteArray& aStr);

  /** Returns a decoded version of the body from the current content transfer
      encoding. The first method returns a null terminated string, the second
      method is meant for binary data, not null is appended */
  virtual QCString bodyDecoded(void) const;
  virtual QByteArray bodyDecodedBinary(void) const;

  /** Number of body parts the message has. This is one for plain messages
      without any attachment. */
  virtual int numBodyParts(void) const;

  /** Get the DwBodyPart at position in aIdx.  Indexing starts at 0.
      If there is no body part at that index, return value will be zero. */
  virtual DwBodyPart * dwBodyPart( int aIdx ) const;

  /** Get the number of the given DwBodyPart.
      If no body part is given, return value will be -1. */
  int partNumber( DwBodyPart * aDwBodyPart ) const;

  /** Get the 1st DwBodyPart.
      If there is no body part, return value will be zero. */
  DwBodyPart * getFirstDwBodyPart() const;

  /** Fill the KMMessagePart structure for a given DwBodyPart. */
  static void bodyPart(DwBodyPart* aDwBodyPart, KMMessagePart* aPart);

  /** Get the body part at position in aIdx.  Indexing starts at 0.
      If there is no body part at that index, aPart will have its
      attributes set to empty values. */
  virtual void bodyPart(int aIdx, KMMessagePart* aPart) const;

  /** Compose a DwBodyPart (needed for adding a part to the message). */
  virtual DwBodyPart* createDWBodyPart(const KMMessagePart* aPart);

  /** Append a DwBodyPart to the message. */
  virtual void addDwBodyPart(DwBodyPart * aDwPart);

  /** Append a body part to the message. */
  virtual void addBodyPart(const KMMessagePart* aPart);

  /** Delete all body parts. */
  virtual void deleteBodyParts(void);

  /** Open a window containing the complete, unparsed, message. */
  virtual void viewSource(const QString& windowCaption, QTextCodec *codec,
					bool fixedfont);

  /** Set "Status" and "X-Status" fields of the message from the
   * internal message status. */
  virtual void setStatusFields(void);

  /** Generates the Message-Id. It uses either the Message-Id suffix
   * defined by the user or the given email address as suffix. The address
   * must be given as addr-spec as defined in RFC 2822.
   */
  static QString generateMessageId( const QString& addr );

  /** Convert '<' into "&lt;" resp. '>' into "&gt;" in order to
    * prevent their interpretation by KHTML.
    * Does *not* use the Qt replace function but runs a very fast C code
    * the same way as lf2crlf() does.
   */
  static QCString html2source( const QCString & src );

  /** Convert LF line-ends to CRLF
   */
  static QCString lf2crlf( const QCString & src );

  /** Strip email address from string. Examples:
   * "Stefan Taferner <taferner@kde.org>" returns "Stefan Taferner"
   * "joe@nowhere.com" returns "joe@nowhere.com". Note that this only
   * returns the first name, e.g. "Peter Test <p@t.de>, Harald Tester <ht@test.de>"
   * returns "Peter Test" */
  static QString stripEmailAddr(const QString& emailAddr);

  /** Return email address from string. Examples:
   * "Stefan Taferner <taferner@kde.org>" returns "taferner@kde.org"
   * "joe@nowhere.com" returns "joe@nowhere.com". Note that this only
   * returns the first address. */
  static QCString getEmailAddr(const QString& emailAddr);

  /** Converts given email address to a nice HTML mailto: anchor.
   * If stripped is TRUE then the visible part of the anchor contains
   * only the name part and not the given emailAddr. */
  static QString emailAddrAsAnchor(const QString& emailAddr,
					 bool stripped=TRUE);

  /** Split a comma separated list of email addresses. */
  static QStringList splitEmailAddrList(const QString&);

  /** Get the default message charset.*/
  static QCString defaultCharset(void);

  /** Get a list of preferred message charsets.*/
  static const QStringList &preferredCharsets(void);

  /** Get the message charset.*/
  virtual QCString charset(void) const;

  /** Set the message charset. */
  virtual void setCharset(const QCString& aStr);

  /** Get the charset the user selected for the message to display */
  virtual QTextCodec* codec(void) const
  { return mCodec; }

  /** Set the charset the user selected for the message to display */
  virtual void setCodec(QTextCodec* aCodec)
  { mCodec = aCodec; }

  /** Allow decoding of HTML for quoting */
  virtual void setDecodeHTML(bool aDecodeHTML)
  { mDecodeHTML = aDecodeHTML; }

  /** Return if the message is complete and not only the header of a message
   * in an IMAP folder */
  virtual bool isComplete()
  { return mIsComplete; }

  /** Set if the message is a complete message */
  virtual void setComplete(bool value)
  { mIsComplete = value; }

  /** Return, if the message should not be deleted */
  virtual bool transferInProgress() { return mTransferInProgress; }

  /** Set that the message shall not be deleted because it is still required */
  virtual void setTransferInProgress(bool value)
  { mTransferInProgress = value; }

  /** Reads config settings from group "KMMessage" and sets all internal
   * variables (e.g. indent-prefix, etc.) */
  static void readConfig(void);

  /** Creates rference string for reply to messages.
   *  reference = original first reference + original last reference + original msg-id
   */
  QCString getRefStr();

  /** Get/set offset in mail folder. */
  virtual unsigned long folderOffset(void) const { return mFolderOffset; }
  void setFolderOffset(unsigned long offs) { if(mFolderOffset != offs) { mFolderOffset=offs; setDirty(TRUE); } }

  /** Get/set filename in mail folder. */
  virtual QString fileName(void) const { return mFileName; }
  void setFileName(const QString& file) { if(mFileName != file) { mFileName=file; setDirty(TRUE); } }

  /** Get/set size of message in the folder including the whole header in
      bytes. Can be 0, if the message is not is a folder
      the setting of mMsgSize = mMsgLength = sz is needed for popFilter*/
  virtual unsigned long msgSize(void) const { return mMsgSize; }
  void setMsgSize(unsigned long sz) { if(mMsgSize != sz) { mMsgSize = sz; setDirty(TRUE); } }

  /** Unlike the above funtion this works also, if the message is not in a
      folder */
  virtual unsigned long msgLength(void) const
    { return (mMsgLength) ? mMsgLength : mMsgSize; }
  void setMsgLength(unsigned long sz) { mMsgLength = sz; }

  /** Status of the message. */
  virtual KMMsgStatus status(void) const { return mStatus; }
  /** Set status and mark dirty. */
  virtual void setStatus(const KMMsgStatus status, int idx = -1);
  virtual void setStatus(const char* s1, const char* s2=0) { KMMsgBase::setStatus(s1, s2); }

    /** Set encryption status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setEncryptionState(const KMMsgEncryptionState status, 
                                  int idx = -1 );
    virtual void setEncryptionState( const char* status, 
        int idx = -1 ) { KMMsgBase::setEncryptionState( status, idx ); }
    
  /** Set signature status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setSignatureState(const KMMsgSignatureState status,
                                 int idx = -1 );
    virtual void setSignatureState( const char* status,
                                    int idx = -1 ) { KMMsgBase::setSignatureState( status, idx ); }

    /** Encryption status of the message. */
    virtual KMMsgEncryptionState encryptionState() const { return mEncryptionState; }

  /** Signature status of the message. */
    virtual KMMsgSignatureState signatureState() const { return mSignatureState; }

  /** Links this message to @p aMsg, setting link type to @p aStatus. */
  void link(const KMMessage *aMsg, KMMsgStatus aStatus);
  /** Returns the information for the Nth link into @p retMsg
   * and @p retStatus. */
  void getLink(int n, ulong *retMsgSerNum, KMMsgStatus *retStatus) const;

  /** Convert wildcards into normal string */
  QString formatString(const QString&) const;

protected:
  void assign( const KMMessage& other );

  QString mDrafts;

protected:
  DwMessage* mMsg;
  bool       mNeedsAssembly, mIsComplete, mTransferInProgress, mDecodeHTML;
  static int sHdrStyle;
  static QString sForwardStr;
  QTextCodec* mCodec;

  QString mFileName;
  unsigned long mFolderOffset, mMsgSize, mMsgLength;
  time_t mDate;
  KMMsgStatus mStatus;
  unsigned long mMsgSerNum;
  KMMsgEncryptionState mEncryptionState;
  KMMsgSignatureState mSignatureState;
};

typedef KMMessage* KMMessagePtr;

#endif /*kmmessage_h*/
