/* Message info describing a messages in a folder
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsginfo_h
#define kmmsginfo_h

#include "kmmsgbase.h"

class KMMessage;

#define KMMsgInfoInherited KMMsgBase

class KMMsgInfo: public KMMsgBase
{
public:
  KMMsgInfo(KMFolder* parent=NULL);
  virtual ~KMMsgInfo();

  /** Initialize from index string and set dirty flag to FALSE. */
  virtual void fromIndexString(const QCString& str, bool toUtf8);

  /** Initialize with given values and set dirty flag to FALSE. */
  virtual void init(const QString& subject, const QString& from,
                    const QString& to, time_t date,
		    KMMsgStatus status, const QString& xmark,
		    const QString& replyToId, const QString& msgId,
		    unsigned long folderOffset=0, unsigned long msgSize=0);

  /** Inherited methods (see KMMsgBase for description): */
  virtual QString subject(void) const;
  virtual QString fromStrip(void) const;
  virtual QString toStrip(void) const;
  virtual QString xmark(void) const;
  virtual QString replyToIdMD5(void) const;
  virtual QString msgIdMD5(void) const;
  virtual void setSubject(const QString&);
  virtual void setXMark(const QString&);
  virtual void setReplyToIdMD5(const QString&);
  virtual void setMsgIdMD5(const QString&);

  /** Copy operators. */
  KMMsgInfo& operator=(const KMMessage&);
  KMMsgInfo& operator=(const KMMsgInfo&);

protected:
  QString mSubject, mFromStrip, mToStrip, mXMark, mReplyToIdMD5, mMsgIdMD5;
};

typedef KMMsgInfo* KMMsgInfoPtr;

#endif /*kmmsginfo_h*/
