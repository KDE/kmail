// -*- mode: C++; c-file-style: "gnu" -*-
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
#include "isubject.h"

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
  namespace Types {
    class AddrSpec;
    class Address;
    typedef QValueList<Address> AddressList;
    typedef QValueList<AddrSpec> AddrSpecList;
  }
}

namespace KMail {
  class HeaderStrategy;
}

class DwBodyPart;
class DwMediaType;
class DwHeaders;

class partNode;

namespace KMail {
  enum ReplyStrategy { ReplySmart = 0,
                       ReplyAuthor,
                       ReplyList,
                       ReplyAll,
                       ReplyNone };
}

class KMMessage: public KMMsgBase, public KMail::ISubject
{
  friend class KMForwardCommand;    // needed for MIME Digest forward

public:
  // promote some of KMMsgBase's methods to public:
  using KMMsgBase::parent;
  using KMMsgBase::setParent;
  using KMMsgBase::enableUndo; // KMFolder
  using KMMsgBase::setEnableUndo; // dto.
  using KMMsgBase::isRead; // dto.
  using KMMsgBase::isUnread; // dto.
  using KMMsgBase::isNew; // dto.
  using KMMsgBase::isOld;
  using KMMsgBase::isWatched;
  using KMMsgBase::isIgnored;
  using KMMsgBase::setEncryptionStateChar; // KMAcct*
  using KMMsgBase::setSignatureStateChar; // dto.

  /** Straight forward initialization. */
  KMMessage(KMFolder* parent=0);

  /** Constructor from a DwMessage. */
  KMMessage(DwMessage*);

  /** Copy constructor. Does *not* automatically load the message. */
  KMMessage(KMMsgInfo& msgInfo);

  /** Copy constructor. */
  KMMessage( const KMMessage& other );

#if 0 // currently unused
  /** Assignment operator. */
  const KMMessage& operator=( const KMMessage& other ) {
    if( &other == this )
      return *this;
    assign( other );
    return *this;
  }
#endif

  /** Destructor. */
  virtual ~KMMessage();

  /** Get KMMsgBase for this object */
  KMMsgBase & toMsgBase() { return *this; }
  const KMMsgBase & toMsgBase() const { return *this; }

  /** Returns TRUE if object is a real message (not KMMsgInfo or KMMsgBase) */
  bool isMessage() const;

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

  /** Returns TRUE if the message contains an unencrypted copy of itself. */
  bool hasUnencryptedMsg() const { return 0 != mUnencryptedMsg; }

  /** Returns an unencrypted copy of this message or 0 if none exists. */
  KMMessage* unencryptedMsg() const { return mUnencryptedMsg; }

  /** Returns an unencrypted copy of this message or 0 if none exists.
      \note This function removes the internal unencrypted message pointer
      from the message: the process calling takeUnencryptedMsg() must
      delete the returned pointer when no longer needed.
  */
  KMMessage* takeUnencryptedMsg()
  {
    KMMessage* ret = mUnencryptedMsg;
    mUnencryptedMsg = 0;
    return ret;
  }

  /** Mark the message as deleted */
  void del() { setStatus(KMMsgStatusDeleted); }

  /** Undelete the message. Same as touch */
  void undel() { setStatus(KMMsgStatusOld); }

  /** Touch the message - mark it as read */
  void touch() { setStatus(KMMsgStatusOld); }

  /** Create a new message that is a reply to this message, filling all
      required header fields with the proper values. The returned message
      is not stored in any folder. Marks this message as replied. */
  KMMessage* createReply( KMail::ReplyStrategy replyStrategy = KMail::ReplySmart,
                          QString selection=QString::null, bool noQuote=FALSE,
                          bool allowDecryption=TRUE, bool selectionIsBody=FALSE);

  /** Create a new message that is a redirect to this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as replied.
    Redirects differ from forwards so they are forwarded to some other
    user, mail is not changed and the reply-to field is set to
    the email address of the original sender
   */
  KMMessage* createRedirect( const QString &toStr );

  /** Create the forwarded body for the message. */
  QCString createForwardBody();

  /** Create a new message that is a forward of this message, filling all
    required header fields with the proper values. The returned message
    is not stored in any folder. Marks this message as forwarded. */
  KMMessage* createForward();

  /** Create a new message that is a delivery receipt of this message,
      filling required header fileds with the proper values. The
      returned message is not stored in any folder. */
  KMMessage* createDeliveryReceipt() const;

  /** Create a new message that is a MDN for this message, filling all
      required fields with proper values. The returned message is not
      stored in any folder.

      @param a Use AutomaticAction for filtering and ManualAction for
               user-induced events.
      @param d See docs for @see KMime::MDN::DispositionType
      @param m See docs for @see KMime::MDN::DispositionModifier
      @param allowGUI Set to true if this method is allowed to ask the
                      user questions

      @return The notification message or 0, if none should be sent.
   **/
  KMMessage* createMDN( KMime::MDN::ActionMode a,
			KMime::MDN::DispositionType d,
			bool allowGUI=false,
			QValueList<KMime::MDN::DispositionModifier> m=QValueList<KMime::MDN::DispositionModifier>() );

  /** Parse the string and create this message from it. */
  void fromDwString(const DwString& str, bool setStatus=FALSE);
  void fromString(const QCString& str, bool setStatus=FALSE);
  void fromByteArray( const QByteArray & ba, bool setStatus=false );

  /** Return the entire message contents in the DwString. This function
      is *fast* even for large message since it does *not* involve a
      string copy.
  */
  const DwString& asDwString() const;
  const DwMessage *asDwMessage();

  /** Return the entire message contents as a string. This function is
      slow for large message since it involves a string copy. If you
      need the string representation only for a short time
      (i.e. without the chance of calling any function in the
      underlying mimelib, then you should use the @see asByteArray,
      which is more efficient or use the @see asDwString function.
      @see asByteArray
      @see asDwString
  */
  QCString asString() const;

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
  DwMediaType& dwContentType();

  /** Return header as string. */
  QString headerAsString() const;

  /** Returns a decoded body part string to be further processed
    by function asQuotedString().
    THIS FUNCTION WILL BE REPLACED ONCE KMime IS FULLY INTEGRATED
    (khz, June 05 2002)*/
  void parseTextStringFromDwPart( partNode * root,
                                          QCString& parsedString,
                                          const QTextCodec*& codec,
                                          bool& isHTML ) const;

  /** Initialize header fields. Should be called on new messages
    if they are not set manually. E.g. before composing. Calling
    of setAutomaticFields(), see below, is still required. */
  void initHeader(uint identity=0);

  /** Initialize headers fields according to the identity and the transport
    header of the given original message */
  void initFromMessage(const KMMessage *msg, bool idHeaders = TRUE);

  /** @return the UOID of the identity for this message.
      Searches the "x-kmail-identity" header and if that fails,
      searches with @see KPIM::IdentityManager::identityForAddress()
      and if that fails queries the @see #parent() folder for a default.
   **/
  uint identityUoid() const;

  /** Set the from, to, cc, bcc, encrytion etc headers as specified in the
   * given identity. */
  void applyIdentity( uint id );

  /** Removes empty fields from the header, e.g. an empty Cc: or Bcc:
    field. */
  void cleanupHeader();

  /** Set fields that are either automatically set (Message-id)
    or that do not change from one message to another (MIME-Version).
    Call this method before sending *after* all changes to the message
    are done because this method does things different if there are
    attachments / multiple body parts. */
  void setAutomaticFields(bool isMultipart=FALSE);

  /** Get or set the 'Date' header field */
  QString dateStr() const;
  /** Returns the message date in asctime format or an empty string if the
      message lacks a Date header. */
  QCString dateShortStr() const;
  QString dateIsoStr() const;
  time_t date() const;
  void setDate(const QCString& str);
  void setDate(time_t aUnixTime);

  /** Set the 'Date' header field to the current date. */
  void setDateToday();

  /** Get or set the 'To' header field */
  QString to() const;
  void setTo(const QString& aStr);
  QString toStrip() const;

  /** Get or set the 'ReplyTo' header field */
  QString replyTo() const;
  void setReplyTo(const QString& aStr);
  void setReplyTo(KMMessage*);

  /** Get or set the 'Cc' header field */
  QString cc() const;
  void setCc(const QString& aStr);
  QString ccStrip() const;

  /** Get or set the 'Bcc' header field */
  QString bcc() const;
  void setBcc(const QString& aStr);

  /** Get or set the 'Fcc' header field */
  QString fcc() const;
  void setFcc(const QString& aStr);

  /** Get or set the 'Drafts' folder */
  QString drafts() const { return mDrafts; }
  void setDrafts(const QString& aStr);

  /** Get or set the 'From' header field */
  QString from() const;
  void setFrom(const QString& aStr);
  QString fromStrip() const;
  QCString fromEmail() const;

  /** @return The addr-spec of either the Sender: (if one is given) or
   * the first addr-spec in From: */
  QString sender() const;

  /** Get or set the 'Who' header field. The actual field that is
      returned depends on the contents of the owning folders whoField().
      Usually this is 'From', but it can also contain 'To'. */
  QString who() const;

  /** Get or set the 'Subject' header field */
  QString subject() const;
  void setSubject(const QString& aStr);

  /** Calculate strippedSubject */
  void initStrippedSubjectMD5() {};

  /** Get or set the 'X-Mark' header field */
  QString xmark() const;
  void setXMark(const QString& aStr);

  /** Get or set the 'In-Reply-To' header field */
  QString replyToId() const;
  void setReplyToId(const QString& aStr);
  QString replyToIdMD5() const;

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
  QString replyToAuxIdMD5() const;

  /**
    Get a hash of the subject with all prefixes such as Re: removed.
    Used for threading.
  */
  QString strippedSubjectMD5() const;

  /**
    Get a hash of the subject.
    Used for threading.
  */
  QString subjectMD5() const;

  /** Is the subject prefixed by Re: or similar? */
  bool subjectIsPrefixed() const;

  /** Get or set the 'Message-Id' header field */
  QString msgId() const;
  void setMsgId(const QString& aStr);
  QString msgIdMD5() const;

  /** Get or set the references for this message */
  QString references() const;
  void setReferences(const QCString& aStr);

  /** Returns the message ID, useful for followups */
  QCString id() const;

  /** Sets the message serial number. If defaulted to zero, the
    serial number will be assigned using the dictionary. Note that
    unless it is explicitely set the serial number will remain 0
    as long as the mail is not in a folder. */
  void setMsgSerNum(unsigned long newMsgSerNum = 0);

  /** Returns the value of a header field with the given name. If multiple
      header fields with the given name might exist then you should use
      headerFields() instead.
  */
  QString headerField(const QCString& name) const;

  enum HeaderFieldType { Unstructured, Structured, Address };

  /** Set the header field with the given name to the given value.
      If prepend is set to true, the header is inserted at the beginning
      and does not overwrite an existing header field with the same name.
  */
  void setHeaderField( const QCString& name, const QString& value,
                       HeaderFieldType type = Unstructured,
                       bool prepend = false );

  /** Returns a list of the values of all header fields with the given name. */
  QStringList headerFields( const QCString& name ) const;

  /** Returns the raw value of a header field with the given name. If multiple
      header fields with the given name might exist then you should use
      rawHeaderFields() instead.
  */
  QCString rawHeaderField( const QCString & name ) const;

  /** Returns a list of the raw values of all header fields with the given
      name.
  */
  QValueList<QCString> rawHeaderFields( const QCString & field ) const;

  /** Splits the given address list into separate addresses. */
  static KMime::Types::AddressList splitAddrField( const QCString & str );

  /** Returns header address list as string list.
      Valid for the following fields: To, Bcc, Cc, ReplyTo, ResentBcc,
      ResentCc, ResentReplyTo, ResentTo */
  KMime::Types::AddressList headerAddrField(const QCString& name) const;
  KMime::Types::AddrSpecList extractAddrSpecs( const QCString & headerNames ) const;

  /** Remove header field with given name */
  void removeHeaderField(const QCString& name);

  /** Get or set the 'Content-Type' header field
      The member functions that involve enumerated types (ints)
      will work only for well-known types or subtypes. */
  QCString typeStr() const;
  int type() const;
  void setTypeStr(const QCString& aStr);
  void setType(int aType);
  /** Subtype */
  QCString subtypeStr() const;
  int subtype() const;
  void setSubtypeStr(const QCString& aStr);
  void setSubtype(int aSubtype);
  /** add or change a parameter of a DwMediaType field */
  static void setDwMediaTypeParam( DwMediaType &mType,
                                   const QCString& attr,
                                   const QCString& val );
  /** add or change a parameter of the Content-Type field */
  void setContentTypeParam(const QCString& attr, const QCString& val);

  /** get the DwHeaders
      (make sure to call setNeedsAssembly() function after directly
       modyfying internal data like the headers) */
  DwHeaders& headers() const;

  /** tell the message that internal data were changed
      (must be called after directly modifying message structures
       e.g. when like changing header information by accessing
       the header via headers() function) */
  void setNeedsAssembly();

  /** Get or set the 'Content-Transfer-Encoding' header field
      The member functions that involve enumerated types (ints)
      will work only for well-known encodings. */
  QCString contentTransferEncodingStr() const;
  int  contentTransferEncoding() const;
  void setContentTransferEncodingStr(const QCString& aStr);
  void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  QCString cteStr() const { return contentTransferEncodingStr(); }
  int cte() const { return contentTransferEncoding(); }
  void setCteStr(const QCString& aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }

  /** Sets this body part's content to @p str. @p str is subject to
      automatic charset and CTE detection.
   **/
  void setBodyFromUnicode( const QString & str );

  /** Returns the body part decoded to unicode.
   **/
  QString bodyToUnicode(const QTextCodec* codec=0) const;

  /** Get the message body. Does not decode the body. */
  QCString body() const;

  /** Set the message body. Does not encode the body. */
  void setBody(const QCString& aStr);

  /** Hack to enable structured body parts to be set as flat text... */
  void setMultiPartBody( const QCString & aStr );

  /** Set the message body, encoding it according to the current content
      transfer encoding. The first method for null terminated strings,
      the second for binary data */
  void setBodyEncoded(const QCString& aStr);
  void setBodyEncodedBinary(const QByteArray& aStr);

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
  void setBodyAndGuessCte( const QByteArray& aBuf,
                                   QValueList<int>& allowedCte,
                                   bool allow8Bit = false,
                                   bool willBeSigned = false );
  void setBodyAndGuessCte( const QCString& aBuf,
                                   QValueList<int>& allowedCte,
                                   bool allow8Bit = false,
                                   bool willBeSigned = false );

  /** Returns a decoded version of the body from the current content transfer
      encoding. The first method returns a null terminated string, the second
      method is meant for binary data, not null is appended */
  QCString bodyDecoded() const;
  QByteArray bodyDecodedBinary() const;

  /** Number of body parts the message has. This is one for plain messages
      without any attachment. */
  int numBodyParts() const;

  /** Return the first DwBodyPart matching a given Content-Type
      or zero, if no found. */
  DwBodyPart * findDwBodyPart( int type, int subtype ) const;

  /** Return the first DwBodyPart matching a given partSpecifier
      or zero, if no found. */
  DwBodyPart* findDwBodyPart( DwBodyPart* part, const QString & partSpecifier );

  /** Get the DwBodyPart at position in aIdx.  Indexing starts at 0.
      If there is no body part at that index, return value will be zero. */
  DwBodyPart * dwBodyPart( int aIdx ) const;

  /** Get the number of the given DwBodyPart.
      If no body part is given, return value will be -1. */
  int partNumber( DwBodyPart * aDwBodyPart ) const;

  /** Get the 1st DwBodyPart.
      If there is no body part, return value will be zero. */
  DwBodyPart * getFirstDwBodyPart() const;
  DwMessage * getTopLevelPart() const { return mMsg; }

  /** Fill the KMMessagePart structure for a given DwBodyPart.
      If withBody is false the body of the KMMessagePart will be left
      empty and only the headers of the part will be filled in*/
  static void bodyPart(DwBodyPart* aDwBodyPart, KMMessagePart* aPart,
		       bool withBody = true );

  /** Get the body part at position in aIdx.  Indexing starts at 0.
      If there is no body part at that index, aPart will have its
      attributes set to empty values. */
  void bodyPart(int aIdx, KMMessagePart* aPart) const;

  /** Compose a DwBodyPart (needed for adding a part to the message). */
  DwBodyPart* createDWBodyPart(const KMMessagePart* aPart);

  /** Append a DwBodyPart to the message. */
  void addDwBodyPart(DwBodyPart * aDwPart);

  /** Append a body part to the message. */
  void addBodyPart(const KMMessagePart* aPart);

  /** Delete all body parts. */
  void deleteBodyParts();

  /** Set "Status" and "X-Status" fields of the message from the
   * internal message status. */
  void setStatusFields();

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

  /** Returns a normalized address built from the given parts. The normalized
      address is of one the following forms:
      - displayName (comment) <addrSpec>
      - displayName <addrSpec>
      - comment <addrSpec>
      - addrSpec
   */
  static QString normalizedAddress( const QString & displayName,
                                    const QString & addrSpec,
                                    const QString & comment );

  /** Decodes the punycode domain part of the given addr-spec if it's an IDN.
   */
  static QString decodeIDN( const QString & addrSpec );

  /** Encodes the domain part of the given addr-spec in punycode if it's an
      IDN.
   */
  static QString encodeIDN( const QString & addrSpec );

  /** Normalizes all email addresses in the given list and decodes all IDNs.
   */
  static QString normalizeAddressesAndDecodeIDNs( const QString & str );

  /** Normalizes all email addresses in the given list and encodes all IDNs
      in punycode.
   */
  static QString normalizeAddressesAndEncodeIDNs( const QString & str );

  /** Encodes an email address as mailto URL
   */
  static QString encodeMailtoUrl( const QString& str );

  /** Decodes a mailto URL
    */
  static QString decodeMailtoUrl( const QString& url );

  enum AddressParseResult { AddressOk, AddressEmpty, NoAddressSpec,
                            UnbalancedQuote, UnbalancedParens,
                            UnclosedAngleAddr, UnexpectedComma,
                            UnexpectedEnd };
  /** Splits the given address into display name, email address and comment.
      Returns AddressOk if no error was encountered. Otherwise an appropriate
      error code is returned. In case of an error the values of displayName,
      addrSpec and comment are undefined.

   */
  static AddressParseResult splitAddress( const QCString& address,
                                          QCString & displayName,
                                          QCString & addrSpec,
                                          QCString & comment );

  /** This function generates a displayable string from a list of email
      addresses.
      Input : mailbox-list
      Output: comma separated list of display name resp. comment resp.
              address
  */
  static QCString stripEmailAddr(const QCString& emailAddr);

  /** Does the same as the above function. Shouldn't be used.
   */
  static QString stripEmailAddr(const QString& emailAddr);

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

  /** Strips an address from an address list. This is for example used
      when replying to all.
  */
  static QStringList stripAddressFromAddressList( const QString& address,
                                                  const QStringList& addresses );

  /** Strips all the user's addresses from an address list. This is used
      when replying.
  */
  static QStringList stripMyAddressesFromAddressList( const QStringList& list );

  /** Returns true if the given address is contained in the given address list.
  */
  static bool addressIsInAddressList( const QString& address,
                                      const QStringList& addresses );

  /** Expands aliases (distribution lists and nick names) and appends a
      domain part to all email addresses which are missing the domain part.
  */
  static QString expandAliases( const QString& recipients );

  /** Uses the hostname as domain part and tries to determine the real name
      from the entries in the password file.
  */
  static QString guessEmailAddressFromLoginName( const QString& userName );


  /** Get the default message charset.*/
  static QCString defaultCharset();

  /** Get a list of preferred message charsets.*/
  static const QStringList &preferredCharsets();

  /** Replaces every occurrence of "${foo}" in @p s with @see
      headerField("foo") */
  QString replaceHeadersInString( const QString & s ) const;

  /** Get the message charset.*/
  QCString charset() const;

  /** Set the message charset. */
  void setCharset(const QCString& aStr);

  /** Get a @see QTextCodec suitable for this message part */
  const QTextCodec * codec() const;

  /** Set the charset the user selected for the message to display */
  void setOverrideCodec( const QTextCodec* codec ) { mOverrideCodec = codec; }

  /** Allow decoding of HTML for quoting */
  void setDecodeHTML(bool aDecodeHTML)
  { mDecodeHTML = aDecodeHTML; }

  /** Reads config settings from group "KMMessage" and sets all internal
   * variables (e.g. indent-prefix, etc.) */
  static void readConfig();

  /** Creates reference string for reply to messages.
   *  reference = original first reference + original last reference + original msg-id
   */
  QCString getRefStr() const;

  /** Get/set offset in mail folder. */
  off_t folderOffset() const { return mFolderOffset; }
  void setFolderOffset(off_t offs) { if(mFolderOffset != offs) { mFolderOffset=offs; setDirty(TRUE); } }

  /** Get/set filename in mail folder. */
  QString fileName() const { return mFileName; }
  void setFileName(const QString& file) { if(mFileName != file) { mFileName=file; setDirty(TRUE); } }

  /** Get/set size of message in the folder including the whole header in
      bytes. Can be 0, if the message is not in a folder.
      The setting of mMsgSize = mMsgLength = sz is needed for popFilter*/
  size_t msgSize() const { return mMsgSize; }
  void setMsgSize(size_t sz) { if(mMsgSize != sz) { mMsgSize = sz; setDirty(TRUE); } }

  /** Unlike the above function this works also, if the message is not in a
      folder */
  size_t msgLength() const
    { return (mMsgLength) ? mMsgLength : mMsgSize; }
  void setMsgLength(size_t sz) { mMsgLength = sz; }

  /** Get/set size on server */
  size_t msgSizeServer() const;
  void setMsgSizeServer(size_t sz);

  /** Get/set UID */
  ulong UID() const;
  void setUID(ulong uid);

  /** Status of the message. */
  KMMsgStatus status() const { return mStatus; }
  /** Set status and mark dirty. */
  void setStatus(const KMMsgStatus status, int idx = -1);
  void setStatus(const char* s1, const char* s2=0) { KMMsgBase::setStatus(s1, s2); }

  /** Set encryption status of the message. */
  void setEncryptionState(const KMMsgEncryptionState, int idx = -1);

  /** Set signature status of the message. */
  void setSignatureState(const KMMsgSignatureState, int idx = -1);

  void setMDNSentState( KMMsgMDNSentState status, int idx=-1 );

  /** Encryption status of the message. */
  KMMsgEncryptionState encryptionState() const { return mEncryptionState; }

  /** Signature status of the message. */
  KMMsgSignatureState signatureState() const { return mSignatureState; }

  KMMsgMDNSentState mdnSentState() const { return mMDNSentState; }

  /** Links this message to @p aMsg, setting link type to @p aStatus. */
  void link(const KMMessage *aMsg, KMMsgStatus aStatus);
  /** Returns the information for the Nth link into @p retMsg
   * and @p retStatus. */
  void getLink(int n, ulong *retMsgSerNum, KMMsgStatus *retStatus) const;

  /** Convert wildcards into normal string */
  QString formatString(const QString&) const;

  /** Sets the body of the specified part */
  void updateBodyPart(const QString partSpecifier, const QByteArray & data);

  /** Returns the last DwBodyPart that was updated */
  DwBodyPart* lastUpdatedPart() { return mLastUpdated; }

  void updateAttachmentState(DwBodyPart * part = 0);

  /** Returns an mbox message separator line for this message, i.e. a
      string of the form
      "From local@domain.invalid Sat Jun 12 14:00:00 2004\n".
  */
  QCString mboxMessageSeparator();

private:
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
  QString asQuotedString( const QString & headerStr,
			  const QString & indentStr,
			  const QString & selection=QString::null,
			  bool aStripSignature=true,
			  bool allowDecryption=true) const;

  /** Return the textual content of the message as plain text,
      converting HTML to plain text if necessary. */
  QString asPlainText( bool stripSignature, bool allowDecryption ) const;

  void assign( const KMMessage& other );

  QString mDrafts;
  mutable DwMessage* mMsg;
  mutable bool       mNeedsAssembly;
  bool mDecodeHTML;
  static const KMail::HeaderStrategy * sHeaderStrategy;
  static QString sForwardStr;
  const QTextCodec * mOverrideCodec;

  QString mFileName;
  off_t mFolderOffset;
  size_t mMsgSize, mMsgLength;
  time_t mDate;
  KMMsgEncryptionState mEncryptionState;
  KMMsgSignatureState mSignatureState;
  KMMsgMDNSentState mMDNSentState;
  KMMessage* mUnencryptedMsg;
  DwBodyPart* mLastUpdated;
};


#endif /*kmmessage_h*/
