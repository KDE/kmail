/* kmmessage.h: Mime Message Class
 *
 */
#ifndef kmmessage_h
#define kmmessage_h

// for large file support
#include <config.h>
#include <sys/types.h>

#include <mimelib/string.h>
#include "kmmsgbase.h"

#include <kmime_mdn.h>

template <typename T>
class QValueList;

class QStringList;
class QString;
class QTextCodec;
class QStrList;

class KMFolder;
class KMFolderIndex;
class DwMessage;
class KMMessagePart;
class KMMsgInfo;
class KMHeaders;

namespace KMime {
  class CharFreq;
};

namespace KMail {
  class HeaderStrategy;
};

class DwBodyPart;
class DwMediaType;
class DwHeaders;

#define KMMessageInherited KMMsgBase
class KMMessage: public KMMsgBase
{
  friend class KMForwardCommand;    // needed for MIME Digest forward

public:
  /** Straight forward initialization. */
  KMMessage(KMFolderIndex* parent=0);

  /** Constructor from a DwMessage. */
  KMMessage(DwMessage*);

  /** Copy constructor. Does *not* automatically load the message. */
  KMMessage(KMMsgInfo& msgInfo);

  /** Copy constructor. */
  KMMessage( const KMMessage& other );
    //KMMessage( const KMMessage& other,
    //           bool preserveArrivalTime=false  );
    //  note: By setting preserveArrivalTime true you get
    //        a message containing the arrival time of the
    //        old one - this is usefull if this new message
    //        is to replace the old one in the same folder
    // note2: temporarily uncommented this again (khz)

  /** Assignment operator. */
  const KMMessage& operator=( const KMMessage& other ) {
    //const KMMessage& operator=( const KMMessage& other,
    //                          bool preserveArrivalTime=false ) {
    //  note: By setting preserveArrivalTime true you get
    //        a message containing the arrival time of the
    //        old one - this is usefull if this new message
    //        is to replace the old one in the same folder
    // note2: temporarily uncommented this again (khz)
    if( &other == this )
      return *this;
    assign( other ); return *this;
  }

  /** Destructor. */
  virtual ~KMMessage();

  /** Returns TRUE if object is a real message (not KMMsgInfo or KMMsgBase) */
  virtual bool isMessage(void) const;

  /** @return whether the priority: or x-priority headers indicate
       that this message should be considered urgent
   **/
  bool isUrgent() const;


  /** Specifies an unencrypted copy of this message to be stored
      in a separate member variable to allow saving messages in
      unencrypted form that were sent in encrypted form.
      NOTE: Target of this pointer becomes property of KMMessage,
            and will be deleted in the d'tor.
  */
  void setUnencryptedMsg( KMMessage* unencrypted );

  /** Returns TRUE is the massage contains an unencrypted copy of itself. */
  virtual bool hasUnencryptedMsg() const { return 0 != mUnencryptedMsg; }

  /** Returns an unencrypted copy of this message or 0 if none exists. */
  virtual KMMessage* unencryptedMsg() const { return mUnencryptedMsg; }

  /** Returns an unencrypted copy of this message or 0 if none exists.
      \note This functions removed the internal unencrypted message pointer
      from the message: the process calling takeUnencryptedMsg() must
      delete the returned pointer when no longer needed.
  */
  virtual KMMessage* takeUnencryptedMsg()
  {
    KMMessage* ret = mUnencryptedMsg;
    mUnencryptedMsg = 0;
    return ret;
  }

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
				 bool allowDecryption=TRUE, bool selectionIsBody=FALSE);

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
  KMMessage* createDeliveryReceipt() const;

  /** Create a new message that is a MDN for this message, filling all
      required felds with proper values. Th ereturned message is not
      stored in any folder.

      @param a Use AutomaticAction for filtering and ManualAction for
               user-induced events.
      @param d See docs for @ref KMime::MDN::DispositionType
      @param m See docs for @ref KMime::MDN::DispositionModifier
      @param allowGUI Set to true if this method is allowed to ask the
                      user questions

      @return The notification message or 0, if none should be sent.
   **/
  KMMessage* createMDN( KMime::MDN::ActionMode a,
			KMime::MDN::DispositionType d,
			bool allowGUI=false,
			QValueList<KMime::MDN::DispositionModifier> m=QValueList<KMime::MDN::DispositionModifier>() );

  /** Parse the string and create this message from it. */
  virtual void fromDwString(const DwString& str, bool setStatus=FALSE);
  virtual void fromString(const QCString& str, bool setStatus=FALSE);

  /** Return the entire message contents in the DwString. This function
      is *fast* even for large message since it does *not* involve a
      string copy.
  */
  virtual const DwString& asDwString() const;
  virtual const DwMessage *asDwMessage(void);

  /** Return the entire message contents as a string. This function is
      slow for large message since it involves a string copy. If you
      need the string representation only for a short time
      (i.e. without the chance of calling any function in the
      underlying mimelib, then you should use the @ref asByteArray,
      which is more efficient or use the @ref asDwString function.
      @see asByteArray
      @see asDwString
  */
  virtual QCString asString() const;

  /**
   * Return the message contents with the headers that should not be
   * sent stripped off.
   */
  QCString asSendableString() const;

  /**
   * Return the message header with the headers that should not be
   * sent stripped off.
   */
  QCString headerAsSendableString() const;

  /**
   * Remove all private header fields: *Status: and X-KMail-*
   **/
  void removePrivateHeaderFields();

  /** Return reference to Content-Type header for direct manipulation. */
  DwMediaType& dwContentType(void);

  /** Return header as string. */
  virtual QString headerAsString(void) const;

  /** Returns a decoded body part string to be further processed
    by function asQuotedString().
    THIS FUNCTION WILL BE REPLACED ONCE KMime IS FULLY INTEGRATED
    (khz, June 05 2002)*/
  virtual void parseTextStringFromDwPart( DwBodyPart * mainBody,
					  DwBodyPart * firstBodyPart,
                                          QCString& parsedString,
                                          bool& isHTML ) const;

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
  virtual void initHeader(uint identity=0);

  /** Initialize headers fields according to the identity and the transport
    header of the given original message */
  virtual void initFromMessage(const KMMessage *msg, bool idHeaders = TRUE);

  /** @return the UOID of the identity for this message.
      Searches the "x-kmail-identity" header and if that fails,
      searches with @ref IdentityManager::identityForAddress()
      and if that fails queries the @ref #parent() folde for a default.
   **/
  uint identityUoid() const;

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

  /** Calculate strippedSubject */
  virtual void initStrippedSubjectMD5() {};

  /** Check for prefixes @p prefixRegExps in @p str. If none
      is found, @p newPrefix + ' ' is prepended to @p str and the
      resulting string is returned. If @p replace is true, any
      sequence of whitespace-delimited prefixes at the beginning of
      @p str is replaced by @p newPrefix.
  **/
  static QString replacePrefixes( const QString& str,
                                  const QStringList& prefixRegExps,
                                  bool replace,
                                  const QString& newPrefix );

  /** Returns @p str with all "forward" and "reply" prefixes stripped off.
   **/
  static QString stripOffPrefixes( const QString& str );

  /** Check for prefixes @p prefixRegExps in @ref #subject(). If none
      is found, @p newPrefix + ' ' is prepended to the subject and the
      resulting string is returned. If @p replace is true, any
      sequence of whitespace-delimited prefixes at the beginning of
      @ref #subject() is replaced by @p newPrefix
  **/
  QString cleanSubject(const QStringList& prefixRegExps, bool replace,
		       const QString& newPrefix) const;

  /** Return this mails subject, with all "forward" and "reply"
      prefixes removed */
  QString cleanSubject() const;

  /** Return this mails subject, formatted for "forward" mails */
  QString forwardSubject() const;

  /** Return this mails subject, formatted for "reply" mails */
  QString replySubject() const;

  /** Get or set the 'X-Mark' header field */
  virtual QString xmark(void) const;
  virtual void setXMark(const QString& aStr);

  /** Get or set the 'In-Reply-To' header field */
  virtual QString replyToId(void) const;
  virtual void setReplyToId(const QString& aStr);
  virtual QString replyToIdMD5(void) const;

  /** Get the second to last id from the References header
      field. If outgoing messages are not kept in the same
      folder as incoming ones, this will be a good place to
      thread the message beneath.
      bob               <- second to last reference points to this
       |_kmailuser      <- not in our folder, but Outbox
           |_bob        <- In-Reply-To points to our mail above

      Thread like this:
      bob
       |_bob

      using replyToAuxIdMD5
    */
  virtual QString replyToAuxIdMD5() const;

  /**
    Get a hash of the subject with all prefixes such as Re: removed.
    Used for threading.
  */
  virtual QString strippedSubjectMD5() const;

  /** Is the subject prefixed by Re: or similar? */
  virtual bool subjectIsPrefixed() const;
  
  /** Get or set the 'Message-Id' header field */
  virtual QString msgId(void) const;
  virtual void setMsgId(const QString& aStr);
  virtual QString msgIdMD5(void) const;

  /** Get or set the references for this message */
  virtual QString references() const;
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

  /** Get a raw header field */
  QCString rawHeaderField( const QCString & name ) const;

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

  /** get the DwHeaders
      (make sure to call setNeedsAssembly() function after directly
       modyfying internal data like the headers) */
  virtual DwHeaders& headers() const;

  /** tell the message that internal data were changed
      (must be called after directly modifying message structures
       e.g. when like changing header information by accessing
       the header via headers() function) */
  void setNeedsAssembly(void);

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

  /** Hack to enable structured body parts to be set as flat text... */
  void setMultiPartBody( const QCString & aStr );

  /** Set the message body, encoding it according to the current content
      transfer encoding. The first method for null terminated strings,
      the second for binary data */
  virtual void setBodyEncoded(const QCString& aStr);
  virtual void setBodyEncodedBinary(const QByteArray& aStr);

  /** Returns a list of content-transfer-encodings that can be used with
      the given result of the character frequency analysis of a message or
      message part under the given restrictions. */
  static QValueList<int> determineAllowedCtes( const KMime::CharFreq& cf,
                                               bool allow8Bit,
                                               bool willBeSigned );

  /** Sets body, encoded in the best fitting
    content-transfer-encoding, which is determined by character
    frequency count.

    @param aBuf       input buffer
    @param allowedCte return: list of allowed cte's
    @param allow8Bit  whether "8bit" is allowed as cte.
    @param willBeSigned whether "7bit"/"8bit" is allowed as cte according to RFC 3156
  */
  virtual void setBodyAndGuessCte( const QByteArray& aBuf,
                                   QValueList<int>& allowedCte,
                                   bool allow8Bit = false,
                                   bool willBeSigned = false );
  virtual void setBodyAndGuessCte( const QCString& aBuf,
                                   QValueList<int>& allowedCte,
                                   bool allow8Bit = false,
                                   bool willBeSigned = false );

  /** Returns a decoded version of the body from the current content transfer
      encoding. The first method returns a null terminated string, the second
      method is meant for binary data, not null is appended */
  virtual QCString bodyDecoded(void) const;
  virtual QByteArray bodyDecodedBinary(void) const;

  /** Number of body parts the message has. This is one for plain messages
      without any attachment. */
  virtual int numBodyParts(void) const;

  /** Return the first DwBodyPart matching a given Content-Type
      or zero, if no found. */
  virtual DwBodyPart * findDwBodyPart( int type, int subtype ) const;

  /** Get the DwBodyPart at position in aIdx.  Indexing starts at 0.
      If there is no body part at that index, return value will be zero. */
  virtual DwBodyPart * dwBodyPart( int aIdx ) const;

  /** Get the number of the given DwBodyPart.
      If no body part is given, return value will be -1. */
  int partNumber( DwBodyPart * aDwBodyPart ) const;

  /** Get the 1st DwBodyPart.
      If there is no body part, return value will be zero. */
  DwBodyPart * getFirstDwBodyPart() const;

  /** Fill the KMMessagePart structure for a given DwBodyPart.
      Iff withBody is false the body of the KMMessagePart will be left
      empty and only the headers of the part will be filled in*/
  static void bodyPart(DwBodyPart* aDwBodyPart, KMMessagePart* aPart,
		       bool withBody = true );

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
  virtual void viewSource(const QString& windowCaption, const QTextCodec *codec,
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

  /** Encodes an email address as mailto URL
   */
  static QString encodeMailtoUrl( const QString& str );

  /** Decodes a mailto URL
    */
  static QString decodeMailtoUrl( const QString& url );

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

  /** Quotes the following characters which have a special meaning in HTML:
   * '<'  '>'  '&'  '"'. Additionally '\n' is converted to "<br />" if
   * @p removeLineBreaks is false. If @p removeLineBreaks is true, then
   * '\n' is removed. Last but not least '\r' is removed.
   */
  static QString quoteHtmlChars( const QString& str,
                                 bool removeLineBreaks = false );

  /** Converts the email address(es) to (a) nice HTML mailto: anchor(s).
   * If stripped is TRUE then the visible part of the anchor contains
   * only the name part and not the given emailAddr.
   */
  static QString emailAddrAsAnchor(const QString& emailAddr,
					 bool stripped=TRUE);

  /** Split a comma separated list of email addresses. */
  static QStringList splitEmailAddrList(const QString&);

  /** Get the default message charset.*/
  static QCString defaultCharset(void);

  /** Get a list of preferred message charsets.*/
  static const QStringList &preferredCharsets(void);

  /** Replaces every occurrence of "${foo}" in @p s with @ref
      headerField("foo") */
  QString replaceHeadersInString( const QString & s ) const;

  /** Get the message charset.*/
  virtual QCString charset(void) const;

  /** Set the message charset. */
  virtual void setCharset(const QCString& aStr);

  /** Get the charset the user selected for the message to display */
  virtual const QTextCodec* codec(void) const
  { return mCodec; }

  /** Set the charset the user selected for the message to display */
  virtual void setCodec(const QTextCodec* aCodec)
  { mCodec = aCodec; }

  /** Allow decoding of HTML for quoting */
  void setDecodeHTML(bool aDecodeHTML)
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
  virtual void setTransferInProgress(bool value);

  /** Reads config settings from group "KMMessage" and sets all internal
   * variables (e.g. indent-prefix, etc.) */
  static void readConfig(void);

  /** Creates reference string for reply to messages.
   *  reference = original first reference + original last reference + original msg-id
   */
  QCString getRefStr() const;

  /** Get/set offset in mail folder. */
  virtual off_t folderOffset(void) const { return mFolderOffset; }
  void setFolderOffset(off_t offs) { if(mFolderOffset != offs) { mFolderOffset=offs; setDirty(TRUE); } }

  /** Get/set filename in mail folder. */
  virtual QString fileName(void) const { return mFileName; }
  void setFileName(const QString& file) { if(mFileName != file) { mFileName=file; setDirty(TRUE); } }

  /** Get/set size of message in the folder including the whole header in
      bytes. Can be 0, if the message is not in a folder.
      The setting of mMsgSize = mMsgLength = sz is needed for popFilter*/
  virtual size_t msgSize(void) const { return mMsgSize; }
  void setMsgSize(size_t sz) { if(mMsgSize != sz) { mMsgSize = sz; setDirty(TRUE); } }

  /** Unlike the above funtion this works also, if the message is not in a
      folder */
  virtual size_t msgLength(void) const
    { return (mMsgLength) ? mMsgLength : mMsgSize; }
  void setMsgLength(size_t sz) { mMsgLength = sz; }

  /** Status of the message. */
  virtual KMMsgStatus status(void) const { return mStatus; }
  /** Set status and mark dirty. */
  virtual void setStatus(const KMMsgStatus status, int idx = -1);
  virtual void setStatus(const char* s1, const char* s2=0) { KMMsgBase::setStatus(s1, s2); }

  /** Set encryption status of the message. */
  virtual void setEncryptionState(const KMMsgEncryptionState, int idx = -1);

  /** Set signature status of the message. */
  virtual void setSignatureState(const KMMsgSignatureState, int idx = -1);

  virtual void setMDNSentState( KMMsgMDNSentState status, int idx=-1 );

  /** Encryption status of the message. */
  virtual KMMsgEncryptionState encryptionState() const { return mEncryptionState; }

  /** Signature status of the message. */
  virtual KMMsgSignatureState signatureState() const { return mSignatureState; }

  virtual KMMsgMDNSentState mdnSentState() const { return mMDNSentState; }

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
  mutable DwMessage* mMsg;
  mutable bool       mNeedsAssembly;
  bool mIsComplete, mDecodeHTML;
  int mTransferInProgress;
  static const KMail::HeaderStrategy * sHeaderStrategy;
  static QString sForwardStr;
  const QTextCodec* mCodec;

  QString mFileName;
  off_t mFolderOffset;
  size_t mMsgSize, mMsgLength;
  time_t mDate;
  KMMsgStatus mStatus;
  unsigned long mMsgSerNum;
  KMMsgEncryptionState mEncryptionState;
  KMMsgSignatureState mSignatureState;
  KMMsgMDNSentState mMDNSentState;
  KMMessage* mUnencryptedMsg;
};

typedef KMMessage* KMMessagePtr;

#endif /*kmmessage_h*/
