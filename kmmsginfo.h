/* Message info describing a messages in a folder
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsginfo_h
#define kmmsginfo_h

#include "kmmessage.h"

#include <qarray.h>
#include <qstring.h>
#include <time.h>

class KMFolder;

class KMMsgInfo
{
  friend class KMFolder;
  friend class KMMessage;

public:
  KMMsgInfo() { mMsg=NULL; mDirty=FALSE; }
  ~KMMsgInfo();

  /** Init object. Second version takes a status string that is parsed. */
  void init(KMMessage::Status, unsigned long offset, unsigned long size,
	    KMMessage* msg=NULL);
  void init(const char* statusStr, unsigned long offset, unsigned long size,
	    KMMessage* msg=NULL);

  /** Init from string. Format is "%c %lu %lu": status, offset, size */
  void fromString(const char* str);

  /** Convert to string using above's format */
  const char* asString(void) const;

  /** Return offset in mail folder */
  unsigned long offset(void) const { return mOffset; }

  /** Return size of message including the whole header in bytes */
  unsigned long size(void) const { return mSize; }
  void setSize(unsigned long sz) { mSize = sz; }

  /** The status is only a copy of the status in the message and does not
    need to be up to date if the message is loaded */
  KMMessage::Status status(void) { return mStatus; }

  /** Pointer to the message object if loaded. */
  KMMessage* msg(void) { return mMsg; }

  /** Set message pointer. */
  void setMsg(KMMessage* m) { mMsg = m; }

  /** Delete message if any and set pointer to NULL. */
  void deleteMsg(void);

  /** The index contains the important header fields. So it is not
   necessary to parse the contents in order to display the list of
   messages. */
  const char* subject(void) const { return mSubject; }
  const char* from(void) const { return mFrom; }
  const time_t date(void) const { return mDate; }
  const char* dateStr(void) const;

  /** Compare with other message info by subject. Returns -1/0/1 like strcmp.*/
  int compareBySubject(const KMMsgInfo* other) const;

  /** Compare with other message info by date. Returns -1/0/1 like strcmp.*/
  int compareByDate(const KMMsgInfo* other) const;

  /** Compare with other message info by from. Returns -1/0/1 like strcmp.*/
  int compareByFrom(const KMMsgInfo* other) const;

  /** Returns TRUE if message info changed since last folder-sync. */
  bool dirty(void) const { return mDirty; }

protected:
  /** Set status and mark as dirty. */
  void setStatus(KMMessage::Status);
  void setStatus(const char*);

  /** Set subject/from/date and mark as dirty. */
  void setSubject(const char*);
  void setFrom(const char*);
  void setDate(const char*);
  void setDate(time_t aUnixTime);

  // strip all trailing blanks from given string starting from given max size
  void stripBlanks(char*,int) const;

  // skip leading keyword if keyword has given character at it's end 
  // (e.g. ':' or ',') and skip the then following blanks (if any) too
  const char* skipKeyword(const char* str, char sepChar) const;

  // set msg-info dirty flag (need write to disk)
  void setDirty(bool df) { mDirty = df; }

  // Size of the toc line in bytes.
  static int recSize(void) { return 220; }

  KMMessage::Status mStatus;
  unsigned long     mOffset;
  unsigned long     mSize;
  KMMessage*        mMsg;
  char              mSubject[80]; // QStrings do not get initialized 
  char              mFrom[80];    // in KMMsgInfoList !
  time_t	    mDate;
  bool              mDirty;
};

typedef QArray<KMMsgInfo> KMMsgInfoList;

#endif /*kmmsginfo_h*/
