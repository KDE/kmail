/* Virtual base class for messages and message infos
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsgbase_h
#define kmmsgbase_h

#include <qstring.h>
#include <time.h>

class QCString;
class QStringList;
class QTextCodec;
class KMFolder;


typedef enum
{
    KMMsgStatusUnknown=' ',
    KMMsgStatusNew='N',
    KMMsgStatusUnread='U',
    KMMsgStatusRead='R',
    KMMsgStatusOld='O',
    KMMsgStatusDeleted='D',
    KMMsgStatusReplied='A',
    KMMsgStatusForwarded='F',
    KMMsgStatusQueued='Q',
    KMMsgStatusSent='S',
    KMMsgStatusFlag='G'
} KMMsgStatus;



/** Flags for the encryption state. */
typedef enum
{
    KMMsgEncryptionStateUnknown=0,
    KMMsgNotEncrypted=1,
    KMMsgPartiallyEncrypted=2,
    KMMsgFullyEncrypted=3
} KMMsgEncryptionState;

/** Flags for the signature state. */
typedef enum
{
    KMMsgSignatureStateUnknown=0,
    KMMsgNotSigned=1,
    KMMsgPartiallySigned=2,
    KMMsgFullySigned=3
} KMMsgSignatureState;



class KMMsgBase
{
public:
  KMMsgBase(KMFolder* p=NULL);
  virtual ~KMMsgBase();

  /** Convert the given message status to a string. */
  static const char* statusToStr(KMMsgStatus aStatus);

  /** Return owning folder. */
  KMFolder* parent(void) const { return mParent; }

  /** Set owning folder. */
  void setParent(KMFolder* p) { mParent=p; }

  /** Returns TRUE if object is a real message (not KMMsgInfo or KMMsgBase) */
  virtual bool isMessage(void) const;

  /** Returns TRUE if status is new or unread. */
  virtual bool isUnread(void) const;

  /** Returns TRUE if status is new. */
  virtual bool isNew(void) const;

  /** Status of the message. */
  virtual KMMsgStatus status(void) const = 0;

  /** Set status and mark dirty.  Optional optimization: @p idx may
   * specify the index of this message within the parent folder. */
  virtual void setStatus(const KMMsgStatus status, int idx = -1);
  virtual void setStatus(const char* statusField, const char* xstatusField=0);


  /** Encryption status of the message. */
  virtual KMMsgEncryptionState encryptionState() const { return mEncryptionState; }

  /** Signature status of the message. */
  virtual KMMsgSignatureState signatureState() const { return mSignatureState; }

  /** Set encryption status of the message. */
  virtual void setEncryptionState(const KMMsgEncryptionState status) { mEncryptionState = status; }

  /** Set signature status of the message. */
  virtual void setSignatureState(const KMMsgSignatureState status) { mSignatureState = status; }


  /** Important header fields of the message that are also kept in the index. */
  virtual QString subject(void) const = 0;
  virtual QString fromStrip(void) const = 0;
  virtual QString toStrip(void) const = 0;
  virtual QString replyToIdMD5(void) const = 0;
  virtual QString msgIdMD5(void) const = 0;
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

  /** Return contents as index string. This string is of indexStringLength() size */
  const uchar *asIndexString(int &len) const;

  /** Get/set offset in mail folder. */
  virtual unsigned long folderOffset(void) const = 0;
  virtual void setFolderOffset(unsigned long offs) = 0;

  /** Get/set msg filename */
  virtual QString fileName(void) const = 0;
  virtual void setFileName(const QString& filename) = 0;

  /** Get/set size of message including the whole header in bytes. */
  virtual unsigned long msgSize(void) const = 0;
  virtual void setMsgSize(unsigned long sz) = 0;

  /** offset into index file */
  virtual void setIndexOffset(long off) { mIndexOffset = off; }
  virtual long indexOffset() const { return mIndexOffset; }

  /** size in index file */
  virtual void setIndexLength(short len) { mIndexLength = len; }
  virtual short indexLength() const { return mIndexLength; }

  /** Skip leading keyword if keyword has given character at it's end
   * (e.g. ':' or ',') and skip the then following blanks (if any) too.
   * If keywordFound is specified it will be TRUE if a keyword was skipped
   * and FALSE otherwise. */
  static QString skipKeyword(const QString& str, QChar sepChar=':',
				 bool* keywordFound=NULL);

  /** Return a QTextCodec for the specified charset.
   * This function is a bit more tolerant, than QTextCodec::codecForName */
  static QTextCodec* codecForName(const QCString& _str);

  /** Convert all non-ascii characters to question marks 
    * If ok is non-null, *ok will be set to true if all characters
    * where ascii, *ok will be set to false otherwise */
  static const QCString toUsAscii(const QString& _str, bool *ok=0);

  /** Return a list of the supported encodings */
  static QStringList supportedEncodings(bool usAscii);

  /** Copy all values from other to this object. */
  void assign(const KMMsgBase* other);

  /** Assignment operator that simply calls assign(). */
  KMMsgBase& operator=(const KMMsgBase& other);

  /** Copy constructor that simply calls assign(). */
  KMMsgBase( const KMMsgBase& other );

  /** En-/decode given string to/from quoted-printable. */
  static QCString decodeQuotedPrintable(const QCString& str);
  static QCString encodeQuotedPrintable(const QCString& str);

  /** En/-decode given string to/from Base64. */
  static QCString decodeBase64(const QCString& str);
  static QCString encodeBase64(const QCString& str);

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

  /**
   * Find out preferred charset for 'text'.
   * First @p encoding is tried and if that one is not suitable,
   * the encodings in @p encodingList are tried.
   */
  static QCString autoDetectCharset(const QCString &encoding, const QStringList &encodingList, const QString &text);

  /** Returns the message serial number for the message. */
  virtual unsigned long getMsgSerNum() const;

protected:
  KMFolder* mParent;
  bool mDirty;
  long mIndexOffset;
  short mIndexLength;

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
    MsgStatusPart = 8,
    MsgSizePart = 9,
    MsgDatePart = 10,
    MsgFilePart = 11
  };
  /** access to long msgparts */
  unsigned long getLongPart(MsgPartType) const; 
  /** access to string msgparts */
  QString getStringPart(MsgPartType) const;
  /** sync'ing just one KMMsgBase */
  bool syncIndexString() const;

private:
  KMMsgEncryptionState mEncryptionState;
  KMMsgSignatureState mSignatureState;
};

typedef KMMsgBase* KMMsgBasePtr;

#endif /*kmmsgbase_h*/
