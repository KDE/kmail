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

  /** Get or set the message body */
  const QString body(void) const;
  void setBody(const QString aStr);

  /** Returns body as decoded string. Assumes that content-transfer-encoding
    contains the correct encoding. */
  virtual const QString bodyDecoded(void) const;

  /** Sets body, encoded according to the content-transfer-encoding. */
  virtual void setEncodedBody(const QString aStr);

  /** Get or set name parameter */
  const QString name(void) const;
  void setName(const QString aStr);

  /** Get or set the 'Content-Type' header field
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

  /** Set the 'Content-Type' by mime-magic from the contents of the body.
    If autoDecode is TRUE the decoded body will be used for mime type
    determination (this does not change the body itself). */
  void magicSetType(bool autoDecode=TRUE);

  /** Tries to find a good icon for the 'Content-Type' by scanning
    the installed mimelnk files. Returns the found icon. If no matching
    icon is found, "unknown.xpm" is returned. */
  const QString iconName(void) const;

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
 
protected:
  QString mType;
  QString mSubtype;
  QString mCte;
  QString mContentDescription;
  QString mContentDisposition;
  QString mBody;
  QString mName;
};


#endif /*kmmsgpart_h*/
