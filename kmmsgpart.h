/* part of a mime multi-part message
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsgpart_h
#define kmmsgpart_h

#include <mimelib/string.h>
#include <qstring.h>

class KMMessagePart
{
public:
  KMMessagePart();
  virtual ~KMMessagePart();

  /** 
   Get or set the 'Content-Type' header field
   The member functions that involve enumerated types (ints)
   will work only for well-known types or subtypes. */
  const QString typeStr(void) const;
  int type(void) const;
  void setTypeStr(const QString aStr);
  void setType(int aType);
  // Subtype
  const QString subtypeStr(void) const;
  int subtype(void) const;
  void setSubtypeStr(const QString aStr);
  void setSubtype(int aSubtype);

  /** Get or set the 'Content-Transfer-Encoding' header field
    The member functions that involve enumerated types (ints)
    will work only for well-known encodings. */
  const QString contentTransferEncodingStr(void) const;
  int  contentTransferEncoding(void) const;
  void setContentTransferEncodingStr(const QString aStr);
  void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  const QString cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const QString aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }

  
  // Get or set the 'Content-Description' header field
  const QString contentDescription() const;
  void setContentDescription(const QString aStr);

  // Get or set the 'Content-Disposition' header field
  const QString contentDisposition() const;
  void setContentDisposition(const QString aStr);
 
  /** Get or set the message body */
  const QString body(long* length_return=0L) const;
  void setBody(const QString aStr);

  /** Get or set name parameter */
  const QString name(void) const;
  void setName(const QString aStr);

protected:
  DwString mType;
  DwString mSubtype;
  DwString mCte;
  DwString mContentDescription;
  DwString mContentDisposition;
  DwString mBody;
  DwString mName;
};


#endif /*kmmsgpart_h*/
