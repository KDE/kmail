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
  virtual void fromIndexString(const QString str);

  /** Initialize with given values and set dirty flag to FALSE. */
  virtual void init(const QString subject, const QString from,
                    const QString to, time_t date,
		    KMMsgStatus status, const QString xmark,
		    unsigned long folderOffset=0, unsigned long msgSize=0);

  /** Inherited methods (see KMMsgBase for description): */
  virtual const QString subject(void) const;
  virtual const QString from(void) const;
  virtual const QString to(void) const;
  virtual const QString xmark(void) const;
  virtual void setSubject(const QString);
  virtual void setFrom(const QString);
  virtual void setXMark(const QString);

  /** Copy operators. */
  KMMsgInfo& operator=(const KMMessage&);
  KMMsgInfo& operator=(const KMMsgInfo&);

protected:
  QString mSubject, mFrom, mTo, mXMark;
};

typedef KMMsgInfo* KMMsgInfoPtr;

#endif /*kmmsginfo_h*/
