/* part of a mime multi-part message
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmmsgpart_h
#define kmmsgpart_h

#include <qstring.h>
#include <qcstring.h>
#include <qdict.h>

template <typename T>
class QValueList;
class QTextCodec;

class KMMessagePart
{
public:
  KMMessagePart();
  KMMessagePart( QDataStream & stream );
  virtual ~KMMessagePart();

  /** Reset to text/plain with 7bit cte and clear all other properties. */
  void clear();

  /** Obtains an independant copy (i.e. without explicitely shared data) of the
      data contained in msgPart. Returns a reference to this message part. */
  void duplicate( const KMMessagePart & msgPart );

  /** Get or set the message body */
  QCString body(void) const;
  void setBody(const QCString &aStr);

  /** Sets this body part's content to @p str. @p str is subject to
      automatic charset and CTE detection.
   **/
  void setBodyFromUnicode( const QString & str );

  /** Returns the body part decoded to unicode.
   **/
  QString bodyToUnicode(const QTextCodec* codec=0) const;

  /** Returns body as decoded string. Assumes that content-transfer-encoding
    contains the correct encoding. This routine is meant for binary data.
    No trailing 0 is appended. */
  virtual QByteArray bodyDecodedBinary(void) const;

  /** Returns body as decoded string. Assumes that content-transfer-encoding
      contains the correct encoding. This routine is meant for text strings! */
  virtual QCString bodyDecoded(void) const;

  /** Sets body, encoded in the best fitting
      content-transfer-encoding, which is determined by character
      frequency count.

      @param aBuf       input buffer
      @param allowedCte return: list of allowed cte's
      @param allow8Bit  whether "8bit" is allowed as cte.
      @param willBeSigned whether "7bit"/"8bit" is allowed as cte according to RFC 3156
  */
  virtual void setBodyAndGuessCte(const QByteArray& aBuf,
				  QValueList<int>& allowedCte,
				  bool allow8Bit = false,
                                  bool willBeSigned = false);
  /** Same for text */
  virtual void setBodyAndGuessCte(const QCString& aBuf,
				  QValueList<int>& allowedCte,
				  bool allow8Bit = false,
                                  bool willBeSigned = false);

  /** Sets body, encoded according to the content-transfer-encoding.
      BEWARE: The entire aStr is used including trailing 0 of text strings! */
  virtual void setBodyEncodedBinary(const QByteArray& aStr);

  /** Sets body, encoded according to the content-transfer-encoding.
      This one is for text strings, the trailing 0 is not used. */
  virtual void setBodyEncoded(const QCString& aStr);

  /** Returns decoded length of body. */
  virtual int decodedSize(void) const;

  /** Get or set the 'Content-Type' header field
   The member functions that involve enumerated types (ints)
   will work only for well-known types or subtypes. */
  QCString originalContentTypeStr(void) const { return mOriginalContentTypeStr; }
  void setOriginalContentTypeStr( const QCString& txt )
  {
    mOriginalContentTypeStr = txt;
  }
  QCString typeStr() const { return mType; }
  void setTypeStr( const QCString & aStr ) { mType = aStr; }
  int type() const;
  void setType(int aType);
  /** Subtype */
  QCString subtypeStr() const { return mSubtype; }
  void setSubtypeStr( const QCString & aStr ) { mSubtype = aStr; }
  int subtype() const;
  void setSubtype(int aSubtype);

  /** Set the 'Content-Type' by mime-magic from the contents of the body.
    If autoDecode is TRUE the decoded body will be used for mime type
    determination (this does not change the body itself). */
  void magicSetType(bool autoDecode=TRUE);

  /** Get or set a custom content type parameter, consisting of an attribute
    name and a corresponding value. */
  QCString parameterAttribute(void) const;
  QString parameterValue(void) const;
  void setParameter(const QCString &attribute, const QString &value);

  QCString additionalCTypeParamStr(void) const
  {
    return mAdditionalCTypeParamStr;
  }
  void setAdditionalCTypeParamStr( const QCString &param )
  {
    mAdditionalCTypeParamStr = param;
  }

  /** Tries to find a good icon for the 'Content-Type' by scanning
    the installed mimelnk files. Returns the found icon. If no matching
    icon is found, the one for application/octet-stream is returned. */
  QString iconName(const QString &mimeType = QString::null) const;

  /** Get or set the 'Content-Transfer-Encoding' header field
    The member functions that involve enumerated types (ints)
    will work only for well-known encodings. */
  QCString contentTransferEncodingStr(void) const;
  int  contentTransferEncoding(void) const;
  void setContentTransferEncodingStr(const QCString &aStr);
  void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  QCString cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const QCString& aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }


  /** Get or set the 'Content-Description' header field */
  QString contentDescription() const;
  QCString contentDescriptionEncoded() const { return mContentDescription; }
  void setContentDescription(const QString &aStr);

  /** Get or set the 'Content-Disposition' header field */
  QCString contentDisposition() const { return mContentDisposition; }
  void setContentDisposition( const QCString & cd ) { mContentDisposition = cd; }

  /** Get the message part charset.*/
  QCString charset() const { return mCharset; }

  /** Set the message part charset. */
  void setCharset( const QCString & c );

  /** Get a @see QTextCodec suitable for this message part */
  const QTextCodec * codec() const;

  /** Get or set name parameter */
  QString name() const { return mName; }
  void setName( const QString & name ) { mName = name; }

  /** Returns name of filename part of 'Content-Disposition' header field,
      if present. */
  QString fileName(void) const;

  /** Returns the part number */
  QString partSpecifier() const { return mPartSpecifier; }

  /** Sets the part number */
  void setPartSpecifier( const QString & part ) { mPartSpecifier = part; }

  /** If this part is complete (contains a body) */
  bool isComplete() { return (!mBody.isNull()); }

  /** Returns the parent part */
  KMMessagePart* parent() { return mParent; }

  /** Set the parent of this part */
  void setParent( KMMessagePart* part ) { mParent = part; }

  /** Returns true if the headers should be loaded */
  bool loadHeaders() { return mLoadHeaders; }

  /** Set to true if the headers should be loaded */
  void setLoadHeaders( bool load ) { mLoadHeaders = load; }

  /** Returns true if the part itself (as returned by kioslave) should be loaded */
  bool loadPart() { return mLoadPart; }

  /** Set to true if the part itself should be loaded */
  void setLoadPart( bool load ) { mLoadPart = load; }

protected:
  QCString mOriginalContentTypeStr;
  QCString mType;
  QCString mSubtype;
  QCString mCte;
  QCString mContentDescription;
  QCString mContentDisposition;
  QByteArray mBody;
  QCString mAdditionalCTypeParamStr;
  QString mName;
  QCString mParameterAttribute;
  QString mParameterValue;
  QCString mCharset;
  QString mPartSpecifier;
  mutable int mBodyDecodedSize;
  KMMessagePart* mParent;
  bool mLoadHeaders;
  bool mLoadPart;
};


#endif /*kmmsgpart_h*/
