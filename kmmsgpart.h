/* part of a mime multi-part message
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsgpart_h
#define kmmsgpart_h

#include <mimelib/string.h>

class KMMessagePart
{
public:
  KMMessagePart();
  virtual ~KMMessagePart();

  /** 
   Get or set the 'Content-Type' header field
   The member functions that involve enumerated types (ints)
   will work only for well-known types or subtypes.
   */
  const char* typeStr(void) const;
  int type(void) const;
  void setTypeStr(const char* aStr);
  void setType(int aType);
  // Subtype
  const char* subtypeStr(void) const;
  int subtype(void) const;
  void setSubtypeStr(const char* aStr);
  void setSubtype(int aSubtype);

  /** Get or set the 'Content-Transfer-Encoding' header field
    The member functions that involve enumerated types (ints)
    will work only for well-known encodings.
   */
  const char* contentTransferEncodingStr(void) const;
  int  contentTransferEncoding(void) const;
  void setContentTransferEncodingStr(const char* aStr);
  void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names.
   */
  const char* cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const char* aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }

  /** Get or set the message body */
  const char* body(long* length_return=0L) const;
  void setBody(const char* aStr);

protected:
  DwString mType;
  DwString mSubtype;
  DwString mCte;
  DwString mContentDescription;
  DwString mContentDisposition;
  DwString mBody;
};


#endif /*kmmsgpart_h*/
