/* Virtual base class for messages and message infos
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsgbase_h
#define kmmsgbase_h

// for large file support flags
#include <config.h>
#include <sys/types.h>
#include <qstring.h>
#include <time.h>

class QCString;
class QStringList;
class QTextCodec;
class KMFolder;
class KMFolderIndex;

/** The new status format. These can be or'd together. */
enum MsgStatus
{
    KMMsgStatusUnknown =           0x00000000,
    KMMsgStatusNew =               0x00000001,
    KMMsgStatusUnread =            0x00000002,
    KMMsgStatusRead =              0x00000004,
    KMMsgStatusOld =               0x00000008,
    KMMsgStatusDeleted =           0x00000010,
    KMMsgStatusReplied =           0x00000020,
    KMMsgStatusForwarded =         0x00000040,
    KMMsgStatusQueued =            0x00000080,
    KMMsgStatusSent =              0x00000100,
    KMMsgStatusFlag =              0x00000200, // important
    KMMsgStatusWatched =           0x00000400,
    KMMsgStatusIgnored =           0x00000800,
    KMMsgStatusTodo =              0x00001000,
    KMMsgStatusSpam =              0x00002000,
    KMMsgStatusHam =               0x00004000,
    KMMsgStatusHasAttach =         0x00008000,
    KMMsgStatusHasNoAttach =       0x00010000
};

typedef uint KMMsgStatus;

/** The old status format, only one at a time possible. Needed
    for upgrade path purposes. */

typedef enum
{
    KMLegacyMsgStatusUnknown=' ',
    KMLegacyMsgStatusNew='N',
    KMLegacyMsgStatusUnread='U',
    KMLegacyMsgStatusRead='R',
    KMLegacyMsgStatusOld='O',
    KMLegacyMsgStatusDeleted='D',
    KMLegacyMsgStatusReplied='A',
    KMLegacyMsgStatusForwarded='F',
    KMLegacyMsgStatusQueued='Q',
    KMLegacyMsgStatusSent='S',
    KMLegacyMsgStatusFlag='G'
} KMLegacyMsgStatus;



/** Flags for the encryption state. */
typedef enum
{
    KMMsgEncryptionStateUnknown=' ',
    KMMsgNotEncrypted='N',
    KMMsgPartiallyEncrypted='P',
    KMMsgFullyEncrypted='F',
    KMMsgEncryptionProblematic='X'
} KMMsgEncryptionState;

/** Flags for the signature state. */
typedef enum
{
    KMMsgSignatureStateUnknown=' ',
    KMMsgNotSigned='N',
    KMMsgPartiallySigned='P',
    KMMsgFullySigned='F',
    KMMsgSignatureProblematic='X'
} KMMsgSignatureState;

/** Flags for the "MDN sent" state. */
typedef enum
{
    KMMsgMDNStateUnknown = ' ',
    KMMsgMDNNone = 'N',
    KMMsgMDNIgnore = 'I',
    KMMsgMDNDisplayed = 'R',
    KMMsgMDNDeleted = 'D',
    KMMsgMDNDispatched = 'F',
    KMMsgMDNProcessed = 'P',
    KMMsgMDNDenied = 'X',
    KMMsgMDNFailed = 'E'
} KMMsgMDNSentState;

/** Flags for the drag and drop action. */
typedef enum
{
    KMMsgDnDActionMOVE=0,
    KMMsgDnDActionCOPY=1,
    KMMsgDnDActionASK=2
} KMMsgDnDAction;

/** Flags for attachment state */
typedef enum
{
  KMMsgHasAttachment,
  KMMsgHasNoAttachment,
  KMMsgAttachmentUnknown
} KMMsgAttachmentState;


class KMMsgBase
{
public:
  KMMsgBase(KMFolder* p=0);
  virtual ~KMMsgBase();

  /** Return owning storage. */
  KMFolderIndex* storage() const;

  /** Return owning folder. */
  KMFolder* parent() const { return mParent; }

  /** Set owning folder. */
  void setParent(KMFolder* p) { mParent = p; }

  /** Convert the given message status to a string. */
  static QCString statusToStr(const KMMsgStatus status);

  /** Convert the given message status to a string. */
  QString statusToSortRank();

  /** Returns TRUE if object is a real message (not KMMsgInfo or KMMsgBase) */
  virtual bool isMessage(void) const;

  /** Returns TRUE if status is new or unread. */
  virtual bool isUnread(void) const;

  /** Returns TRUE if status is new. */
  virtual bool isNew(void) const;

  /** Returns TRUE if status is unknown. */
  virtual bool isOfUnknownStatus(void) const;

  /** Returns TRUE if status is old. */
  virtual bool isOld(void) const;

  /** Returns TRUE if status is read. */
  virtual bool isRead(void) const;

  /** Returns TRUE if status is deleted. */
  virtual bool isDeleted(void) const;

  /** Returns TRUE if status is replied. */
  virtual bool isReplied(void) const;

  /** Returns TRUE if status is forwarded. */
  virtual bool isForwarded(void) const;

  /** Returns TRUE if status is queued. */
  virtual bool isQueued(void) const;

  /** Returns TRUE if status is sent. */
  virtual bool isSent(void) const;

  /** Returns TRUE if status is important. */
  virtual bool isImportant(void) const;

  /** Returns TRUE if status is watched. */
  virtual bool isWatched(void) const;

  /** Returns TRUE if status is ignored. */
  virtual bool isIgnored(void) const;

  /** Returns TRUE if status is spam. */
  virtual bool isSpam(void) const;

  /** Returns TRUE if status is not spam. */
  virtual bool isHam(void) const;


  /** Status of the message. */
  virtual KMMsgStatus status(void) const = 0;

  /** Set status and mark dirty.  Optional optimization: @p idx may
   * specify the index of this message within the parent folder. */
  virtual void setStatus(const KMMsgStatus status, int idx = -1);
  virtual void toggleStatus(const KMMsgStatus status, int idx = -1);
  virtual void setStatus(const char* statusField, const char* xstatusField=0);

  /** Encryption status of the message. */
  virtual KMMsgEncryptionState encryptionState() const = 0;

  /** Signature status of the message. */
  virtual KMMsgSignatureState signatureState() const = 0;

  /** "MDN send" status of the message. */
  virtual KMMsgMDNSentState mdnSentState() const = 0;

  /** Set "MDN sent" status of the message. */
  virtual void setMDNSentState( KMMsgMDNSentState status, int idx=-1 );

  /** Set encryption status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setEncryptionState(const KMMsgEncryptionState, int idx = -1);

  /** Set signature status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setSignatureState(const KMMsgSignatureState, int idx = -1);

  /** Set encryption status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setEncryptionStateChar( QChar status, int idx = -1 );

  /** Set signature status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setSignatureStateChar( QChar status, int idx = -1 );

  /** Important header fields of the message that are also kept in the index. */
  virtual QString subject(void) const = 0;
  virtual QString fromStrip(void) const = 0;
  virtual QString toStrip(void) const = 0;
  virtual QString replyToIdMD5(void) const = 0;
  virtual QString msgIdMD5(void) const = 0;
  virtual QString replyToAuxIdMD5() const = 0;
  virtual QString strippedSubjectMD5() const = 0;
  virtual bool subjectIsPrefixed() const = 0;
  virtual time_t date(void) const = 0;
  virtual QString dateStr(void) const;
  virtual QString xmark(void) const = 0;

  /** Set date. */
  virtual void setDate(const QCString &aStrDate);
  virtual void setDate(time_t aUnixTime) = 0;

  /** Returns TRUE if changed since last folder-sync. */
  virtual bool dirty(void) const { return mDirty; }

  /** Change dirty flag. */
  void setDirty(bool b) { mDirty = b; }

  /** Set subject/from/date and xmark. */
  virtual void setSubject(const QString&) = 0;
  virtual void setXMark(const QString&) = 0;

  /** Calculate strippedSubject */
  virtual void initStrippedSubjectMD5() = 0;

  /** Return contents as index string. This string is of indexStringLength() size */
  const uchar *asIndexString(int &len) const;

  /** Get/set offset in mail folder. */
  virtual off_t folderOffset(void) const = 0;
  virtual void setFolderOffset(off_t offs) = 0;

  /** Get/set msg filename */
  virtual QString fileName(void) const = 0;
  virtual void setFileName(const QString& filename) = 0;

  /** Get/set size of message including the whole header in bytes. */
  virtual size_t msgSize(void) const = 0;
  virtual void setMsgSize(size_t sz) = 0;

  /** Get/set size of message on server */
  virtual size_t msgSizeServer(void) const = 0;
  virtual void setMsgSizeServer(size_t sz) = 0;

  /** Get/set UID for IMAP */
  virtual ulong UID(void) const = 0;
  virtual void setUID(ulong uid) = 0;

  /** offset into index file */
  virtual void setIndexOffset(off_t off) { mIndexOffset = off; }
  virtual off_t indexOffset() const { return mIndexOffset; }

  /** size in index file */
  virtual void setIndexLength(short len) { mIndexLength = len; }
  virtual short indexLength() const { return mIndexLength; }

  /** Skip leading keyword if keyword has given character at it's end
   * (e.g. ':' or ',') and skip the then following blanks (if any) too.
   * If keywordFound is specified it will be TRUE if a keyword was skipped
   * and FALSE otherwise. */
  static QString skipKeyword(const QString& str, QChar sepChar=':',
				 bool* keywordFound=0);

  /** Return a QTextCodec for the specified charset.
   * This function is a bit more tolerant, than QTextCodec::codecForName */
  static const QTextCodec* codecForName(const QCString& _str);

  /** Convert all non-ascii characters to question marks
    * If ok is non-null, *ok will be set to true if all characters
    * where ascii, *ok will be set to false otherwise */
  static QCString toUsAscii(const QString& _str, bool *ok=0);

  /** Return a list of the supported encodings */
  static QStringList supportedEncodings(bool usAscii);

  /** Copy all values from other to this object. */
  void assign(const KMMsgBase* other);

  /** Assignment operator that simply calls assign(). */
  KMMsgBase& operator=(const KMMsgBase& other);

  /** Copy constructor that simply calls assign(). */
  KMMsgBase( const KMMsgBase& other );

  /** Helper function for encodeRFC2047String */
  static QCString encodeRFC2047Quoted(const QCString& aStr, bool base64);

  /** This function handles both encodings described in RFC2047:
    Base64 ("=?iso-8859-1?b?...?=") and quoted-printable */
  static QString decodeRFC2047String(const QCString& aStr);

  /** Encode given string as described in RFC2047:
    using quoted-printable. */
  static QCString encodeRFC2047String(const QString& aStr,
    const QCString& charset);

  /** Encode given string as described in RFC2231
    (parameters in MIME headers) */
  static QCString encodeRFC2231String(const QString& aStr,
    const QCString& charset);

  /** Decode given string as described in RFC2231 */
  static QString decodeRFC2231String(const QCString& aStr);

  /** Calculate the base64 encoded md5sum (sans the trailing equal
      signs). If @p utf8 is false, uses QString::latin1() to calculate
      the md5sum of, else uses QString::utf8() */
  static QString base64EncodedMD5( const QString & aStr, bool utf8=false );
  static QString base64EncodedMD5( const QCString & aStr );
  static QString base64EncodedMD5( const char * aStr, int len=-1 );

  /**
   * Find out preferred charset for 'text'.
   * First @p encoding is tried and if that one is not suitable,
   * the encodings in @p encodingList are tried.
   */
  static QCString autoDetectCharset(const QCString &encoding, const QStringList &encodingList, const QString &text);

  /** Returns the message serial number for the message. */
  virtual unsigned long getMsgSerNum() const;

  /** If undo for this message should be enabled */
  virtual bool enableUndo() { return mEnableUndo; }
  virtual void setEnableUndo( bool enable ) { mEnableUndo = enable; }

  /** Return if the message is complete and not only the header of a message
   * in an IMAP folder */
  bool isComplete();
  /** Set if the message is a complete message */
  void setComplete(bool value);

  /** Return if the message is ready to be shown */
  bool readyToShow();
  /** Set if the message is ready to be shown */
  void setReadyToShow(bool);

  /** Return, if the message should not be deleted */
  bool transferInProgress();
  /** Set that the message shall not be deleted because it is still required */
  void setTransferInProgress(bool value, bool force = false);

  /** Return if the message has at least one attachment */
  virtual KMMsgAttachmentState attachmentState() const;

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

  /** Reads config settings from group "Composer" and sets all internal
   * variables (e.g. indent-prefix, etc.) */
  static void readConfig();

protected:
  KMFolder* mParent;
  off_t mIndexOffset;
  short mIndexLength;
  bool mDirty;
  bool mEnableUndo;
  mutable KMMsgStatus mStatus;
  // This is kept to provide an upgrade path from the the old single status
  // to the new multiple status scheme.
  mutable KMLegacyMsgStatus mLegacyStatus;

public:
  enum MsgPartType
  {
    MsgNoPart = 0,
    //unicode strings
    MsgFromPart = 1,
    MsgSubjectPart = 2,
    MsgToPart = 3,
    MsgReplyToIdMD5Part = 4,
    MsgIdMD5Part = 5,
    MsgXMarkPart = 6,
    //unsigned long
    MsgOffsetPart = 7,
    MsgLegacyStatusPart = 8,
    MsgSizePart = 9,
    MsgDatePart = 10,
    MsgFilePart = 11,
    MsgCryptoStatePart = 12,
    MsgMDNSentPart = 13,
    //another two unicode strings
    MsgReplyToAuxIdMD5Part = 14,
    MsgStrippedSubjectMD5Part = 15,
    // and another unsigned long
    MsgStatusPart = 16,
    MsgSizeServerPart = 17,
    MsgUIDPart = 18
  };
  /** access to long msgparts */
  off_t getLongPart(MsgPartType) const;
  /** access to string msgparts */
  QString getStringPart(MsgPartType) const;
  /** sync'ing just one KMMsgBase */
  bool syncIndexString() const;
};

#endif /*kmmsgbase_h*/
