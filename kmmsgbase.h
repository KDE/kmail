/* Virtual base class for messages and message infos
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL.
 */
#ifndef kmmsgbase_h
#define kmmsgbase_h

#include <qstring.h>
#include <time.h>

typedef enum
{
    KMMsgStatusUnknown=' ',
    KMMsgStatusNew='N',
    KMMsgStatusUnread='U',
    KMMsgStatusOld='O',
    KMMsgStatusDeleted='D',
    KMMsgStatusReplied='A',
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

  /** Set status and mark dirty. */
  virtual void setStatus(const KMMsgStatus status);
  virtual void setStatus(const char* statusStr);

  /** Important header fields of the message that are also kept in the index. */
  virtual const QString subject(void) const = 0;
  virtual const QString from(void) const = 0;
  virtual time_t date(void) const;
  virtual const QString dateStr(void) const;

  /** Set date. */
  virtual void setDate(const char* aStrDate);
  virtual void setDate(time_t aUnixTime);

  /** Returns TRUE if changed since last folder-sync. */
  bool dirty(void) const { return mDirty; }

  /** Change dirty flag. */
  void setDirty(bool d) { mDirty=d; }

  /** Set subject/from/date and mark dirty. */
  virtual void setSubject(const QString) = 0;
  virtual void setFrom(const QString) = 0;

  /** Return contents as index string. This string is of fixed size
    that can be read with indexStringLength(). */
  virtual const QString asIndexString(void) const;

  /** Returns fixed length of index strings returned by asIndexString(). */
  static int indexStringLength(void);

  /** Get/set offset in mail folder. */
  unsigned long folderOffset(void) const { return mFolderOffset; }
  void setFolderOffset(unsigned long offs) { mFolderOffset=offs; }

  /** Get/set size of message including the whole header in bytes. */
  unsigned long msgSize(void) const { return mMsgSize; }
  void setMsgSize(unsigned long sz) { mMsgSize = sz; }

  /** Compare with other message by Status. Returns -1/0/1 like strcmp.*/
  int compareByStatus(const KMMsgBase* other) const;

  /** Compare with other message by Subject. Returns -1/0/1 like strcmp.*/
  int compareBySubject(const KMMsgBase* other) const;

  /** Compare with other message by Date. Returns -1/0/1 like strcmp.*/
  int compareByDate(const KMMsgBase* other) const;

  /** Compare with other message by From. Returns -1/0/1 like strcmp.*/
  int compareByFrom(const KMMsgBase* other) const;

  /** Skip leading keyword if keyword has given character at it's end 
   * (e.g. ':' or ',') and skip the then following blanks (if any) too.
   * If keywordFound is specified it will be TRUE if a keyword was skipped
   * and FALSE otherwise. */
  static const char* skipKeyword(const QString str, char sepChar=':',
				 bool* keywordFound=NULL);

  /** Copy all values from other to this object. */ 
  void assign(const KMMsgBase* other);

  /** Assignment operator that simply calls assign(). */
  KMMsgBase& operator=(const KMMsgBase& other);

  /** En-/decode given string to/from quoted-printable. */
  static const QString decodeQuotedPrintable(const QString str);
  static const QString encodeQuotedPrintable(const QString str);

  /** Decode given string from possibly quoted-printable encoded
    string. These strings contain parts of the type "=?iso8859-1?Q?...?=".
    These parts are not correct decoded by decodeQuotedPrintable(). 
    Use this method if you want to ensure that a given header field
    is readable. */
  static const QString decodeQuotedPrintableString(const QString str);

  /** En/-decode given string to/from Base64. */
  static const QString decodeBase64(const QString str);
  static const QString encodeBase64(const QString str);

protected:
  KMFolder* mParent;
  unsigned long mFolderOffset, mMsgSize;
  time_t mDate;
  KMMsgStatus mStatus;
  bool mDirty;
};

typedef KMMsgBase* KMMsgBasePtr;

#endif /*kmmsgbase_h*/
