/* Virtual base class for messages and message infos
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsgbase_h
#define kmmsgbase_h

#include <qstring.h>
#include <time.h>
#include <qtextcodec.h>

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
    KMMsgStatusSent='S'
} KMMsgStatus;

class KMFolder;


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

  /** Status of the message. */
  virtual KMMsgStatus status(void) const;

  /** Returns TRUE if status is new or unread. */
  virtual bool isUnread(void) const;

  /** Returns TRUE if status is new. */
  virtual bool isNew(void) const;
    
  /** Set status and mark dirty. */
  virtual void setStatus(const KMMsgStatus status);
  virtual void setStatus(const char* statusField, const char* xstatusField=0);

  /** Important header fields of the message that are also kept in the index. */
  virtual const QString subject(void) const = 0;
  virtual const QString fromStrip(void) const = 0;
  virtual const QString toStrip(void) const = 0;
  virtual const QString replyToIdMD5(void) const = 0;
  virtual const QString msgIdMD5(void) const = 0;
  virtual time_t date(void) const;
  virtual const QString dateStr(void) const;
  virtual const QString xmark(void) const = 0;

  /** Set date. */
  virtual void setDate(const char* aStrDate);
  virtual void setDate(time_t aUnixTime);

  /** Returns TRUE if changed since last folder-sync. */
  bool dirty(void) const { return mDirty; }

  /** Change dirty flag. */
  void setDirty(bool d) { mDirty=d; }

  /** Set subject/from/date and xmark. */
  virtual void setSubject(const QString&) = 0;
  virtual void setXMark(const QString&) = 0;

  /** Return contents as index string. This string is of fixed size
    that can be read with indexStringLength(). */
  virtual const QCString asIndexString(void) const;

  /** Returns fixed length of index strings returned by asIndexString(). */
  static int indexStringLength(void);

  /** Get/set offset in mail folder. */
  unsigned long folderOffset(void) const { return mFolderOffset; }
  void setFolderOffset(unsigned long offs) { mFolderOffset=offs; }

  /** Get/set size of message including the whole header in bytes. */
  unsigned long msgSize(void) const { return mMsgSize; }
  void setMsgSize(unsigned long sz) { mMsgSize = sz; }

  /** Skip leading keyword if keyword has given character at it's end
   * (e.g. ':' or ',') and skip the then following blanks (if any) too.
   * If keywordFound is specified it will be TRUE if a keyword was skipped
   * and FALSE otherwise. */
  static QString skipKeyword(const QString& str, char sepChar=':',
				 bool* keywordFound=NULL);

  /** Return a QTextCodec for the specified charset.
   * This function is a bit more tolerant, than QTextCodec::codecForName */
  static QTextCodec* codecForName(const QString& _str);

  /** Convert all non-ascii characters to question marks */
  static const QCString toUsAscii(const QString& _str);

  /** Copy all values from other to this object. */
  void assign(const KMMsgBase* other);

  /** Assignment operator that simply calls assign(). */
  KMMsgBase& operator=(const KMMsgBase& other);

  /** En-/decode given string to/from quoted-printable. */
  static const QString decodeQuotedPrintable(const QString& str);
  static const QString encodeQuotedPrintable(const QString& str);

  /** Decode given string from possibly quoted-printable encoded
    string. These strings contain parts of the type "=?iso8859-1?Q?...?=".
    These parts are not correct decoded by decodeQuotedPrintable().
    Use this method if you want to ensure that a given header field
    is readable. */
  static const QString decodeQuotedPrintableString(const QString& str);

  /** En/-decode given string to/from Base64. */
  static const QString decodeBase64(const QString& str);
  static const QString encodeBase64(const QString& str);

  /** This function handles both encodings described in RFC2047:
    Base64 ("=?iso-8859-1?b?...?=") and quoted-printable */
  static const QString decodeRFC2047String(const QString& aStr);

  /** Encode given string as described in RFC2047: 
    using quoted-printable. */
  static const QString encodeRFC2047String(const QString& aStr,
    const QString& charset);

  /** Encode given string as described in RFC2231
    (parameters in MIME headers) */
  static const QString encodeRFC2231String(const QString& aStr,
    const QString& charset);

  /** Decode given string as described in RFC2231 */
  static const QString decodeRFC2231String(const QString& aStr);

protected:
  KMFolder* mParent;
  unsigned long mFolderOffset, mMsgSize;
  time_t mDate;
  KMMsgStatus mStatus;
  bool mDirty;
};

typedef KMMsgBase* KMMsgBasePtr;

#endif /*kmmsgbase_h*/
