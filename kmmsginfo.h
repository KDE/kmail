/* Message info describing a messages in a folder
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsginfo_h
#define kmmsginfo_h

#include <config.h>
#include <sys/types.h>
#include "kmmsgbase.h"

class KMMessage;

class KMMsgInfo: public KMMsgBase
{
public:
  KMMsgInfo(KMFolder* parent, off_t off=0, short len=0);
  virtual ~KMMsgInfo();

  /** left for old style index files */
  void compat_fromOldIndexString(const QCString& str, bool toUtf8);


  /** Initialize with given values and set dirty flag to FALSE. */
  virtual void init(const QCString& subject, const QCString& from,
                    const QCString& to, time_t date,
		    KMMsgStatus status, const QCString& xmark,
                    const QCString& replyToId,
                    const QCString& replyToAuxId,
                    const QCString& msgId,
		    KMMsgEncryptionState encryptionState,
		    KMMsgSignatureState signatureState,
		    KMMsgMDNSentState mdnSentState,
		    off_t folderOffset=0, size_t msgSize=0,
            size_t msgSizeServer = 0, ulong UID = 0);

  /** Initialize with given values and set dirty flag to FALSE. */
  virtual void init(const QCString& subject, const QCString& from,
                    const QCString& to, time_t date,
		    KMMsgStatus status, const QCString& xmark,
                    const QCString& replyToId,
                    const QCString& replyToAuxId,
                    const QCString& msgId,
		    const QCString& fileName,
		    KMMsgEncryptionState encryptionState,
		    KMMsgSignatureState signatureState,
		    KMMsgMDNSentState mdnSentState,
		    size_t msgSize=0,
            size_t msgSizeServer = 0, ulong UID = 0);

  /** Inherited methods (see @see KMMsgBase for description): */
  virtual QString subject(void) const;
  virtual QString fromStrip(void) const;
  virtual QString toStrip(void) const;
  virtual QString xmark(void) const;
  virtual QString replyToIdMD5(void) const;
  virtual QString replyToAuxIdMD5() const;
  virtual QString strippedSubjectMD5() const;
  virtual bool subjectIsPrefixed() const;
  virtual QString msgIdMD5(void) const;
  virtual QString fileName(void) const;
  virtual KMMsgStatus status(void) const;
  virtual KMMsgEncryptionState encryptionState() const;
  virtual KMMsgSignatureState signatureState() const;
  virtual KMMsgMDNSentState mdnSentState() const;
  virtual off_t folderOffset(void) const;
  virtual size_t msgSize(void) const;
  virtual size_t msgSizeServer(void) const;
  virtual time_t date(void) const;
  virtual ulong UID(void) const;
  void setMsgSize(size_t sz);
  void setMsgSizeServer(size_t sz);
  void setFolderOffset(off_t offs);
  void setFileName(const QString& file);
  virtual void setStatus(const KMMsgStatus status, int idx = -1);
  virtual void setDate(time_t aUnixTime);
  virtual void setSubject(const QString&);
  virtual void setXMark(const QString&);
  virtual void setReplyToIdMD5(const QString&);
  virtual void setReplyToAuxIdMD5( const QString& );
  virtual void initStrippedSubjectMD5();
  virtual void setMsgIdMD5(const QString&);
  virtual void setEncryptionState( const KMMsgEncryptionState, int idx = -1 );
  virtual void setSignatureState( const KMMsgSignatureState, int idx = -1 );
  virtual void setMDNSentState( const KMMsgMDNSentState, int idx = -1 );
  virtual void setUID(ulong);

  /** Grr.. c++! */
  virtual void setStatus(const char* s1, const char* s2=0) { KMMsgBase::setStatus(s1, s2); }
  virtual void setDate(const char* s1) { KMMsgBase::setDate(s1); }

  virtual bool dirty(void) const;

  /** Copy operators. */
  KMMsgInfo& operator=(const KMMessage&);
  KMMsgInfo& operator=(const KMMsgInfo&);


private:
  // WARNING: Do not add new member variables to the class. Add them to kd
  class KMMsgInfoPrivate;
  KMMsgInfoPrivate *kd;
};

typedef KMMsgInfo* KMMsgInfoPtr;

#endif /*kmmsginfo_h*/
