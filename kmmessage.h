/* kmmessage.h: Mime Message Class
 *
 */
#ifndef kmmessage_h
#define kmmessage_h

#include <mimelib/string.h>
#include "kmmsgbase.h"

class KMFolder;
class DwMessage;
class KMMessagePart;
class KMMsgInfo;

#define KMMessageInherited KMMsgBase
class KMMessage: public KMMsgBase
{
 friend class KMFolder;

public:
  /** Straight forward initialization. */
  KMMessage(KMFolder* parent=NULL);

  /** Copy constructor. Does *not* automatically load the message. */
  KMMessage(const KMMsgInfo& msgInfo);

  /** Destructor. */
  virtual ~KMMessage();

  /** Returns TRUE if object is a real message (not KMMsgInfo or KMMsgBase) */
  virtual bool isMessage(void) const;

  /** Mark the message as deleted */
  void del(void) { setStatus(KMMsgStatusDeleted); }

  /** Undelete the message. Same as touch */
  void undel(void) { setStatus(KMMsgStatusOld); }

  /** Touch the message - mark it as read */
  void touch(void) { setStatus(KMMsgStatusOld); }

  /** Create a new message that is a reply to this message, filling all 
    required header fields with the proper values. The returned message
    is not stored in any folder. */
  virtual KMMessage* createReply(bool replyToAll=FALSE) const;

  /** Create a new message that is a forward of this message, filling all 
    required header fields with the proper values. The returned message
    is not stored in any folder. */
  virtual KMMessage* createForward(void) const;

  /** Parse the string and create this message from it. */
  virtual void fromString(const QString str);

  /** Return the entire message contents as a string. */
  virtual const QString asString(void);

  /** Returns message body with quoting header and indented by the 
    given indentation string. This is suitable for including the message
    in another message of for replies, forwards. The header string is 
    a template where the following fields are replaced with the 
    corresponding values:
	%D: date of this message
	%S: subject of this message
	%F: sender (from) of this message
	%%: a single percent sign  */
  virtual const QString asQuotedString(const QString headerStr, 
				       const QString indentStr) const;

  /** Initialize header fields. Should be called on new messages
    if they are not set manually. E.g. before composing. Calling
    if setAutomaticFields() is still required. */
  virtual void initHeader(void);

  /** Set fields that are either automatically set (Message-id)
   or that do not change from one message to another (MIME-Version).
   Call this method before sending *after* all changes to the message
   are done because this method does things different if there are
   attachments / multiple body parts. */
  virtual void setAutomaticFields(void);
    
  /** Get or set the 'Date' header field */
  virtual const QString dateStr(void) const;
  virtual const QString dateShortStr(void) const;
  virtual time_t date(void) const;
  virtual void setDate(const QString str);
  virtual void setDate(time_t aUnixTime);

  /** Get or set the 'To' header field */
  virtual const QString to(void) const;
  virtual void setTo(const QString aStr);

  /** Get or set the 'ReplyTo' header field */
  virtual const QString replyTo(void) const;
  virtual void setReplyTo(const QString aStr);
  virtual void setReplyTo(KMMessage*);

  /** Get or set the 'Cc' header field */
  virtual const QString cc(void) const;
  virtual void setCc(const QString aStr);

  /** Get or set the 'Bcc' header field */
  virtual const QString bcc(void) const;
  virtual void setBcc(const QString aStr);

  /** Get or set the 'From' header field */
  virtual const QString from(void) const;
  virtual void setFrom(const QString aStr);

  /** Get or set the 'Subject' header field */
  virtual const QString subject(void) const;
  virtual void setSubject(const QString aStr);

  /** Get or set header field with given name */
  virtual const QString headerField(const QString name) const;
  virtual void setHeaderField(const QString name, const QString value);

  /** Get or set the 'Content-Type' header field
   The member functions that involve enumerated types (ints)
   will work only for well-known types or subtypes. */
  virtual const QString typeStr(void) const;
  virtual int type(void) const;
  virtual void setTypeStr(const QString aStr);
  virtual void setType(int aType);
  // Subtype
  virtual const QString subtypeStr(void) const;
  virtual int subtype(void) const;
  virtual void setSubtypeStr(const QString aStr);
  virtual void setSubtype(int aSubtype);

  /** Get or set the 'Content-Transfer-Encoding' header field
    The member functions that involve enumerated types (ints)
    will work only for well-known encodings. */
  virtual const QString contentTransferEncodingStr(void) const;
  virtual int  contentTransferEncoding(void) const;
  virtual void setContentTransferEncodingStr(const QString aStr);
  virtual void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  const QString cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const QString aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }

  /** Get or set the message body */
  virtual const QString body(void) const;
  virtual void setBody(const QString aStr);

  /** Number of body parts the message has. This is one for plain messages
      without any attachment. */
  virtual int numBodyParts(void) const;

  /** Get the body part at position in aIdx.  Indexing starts at 0.
    If there is no body part at that index, aPart will have its
    attributes set to empty values. */
  virtual void bodyPart(int aIdx, KMMessagePart* aPart) const;
    
  /** Set the body part at position in aIdx.  Indexing starts at 0.
    If you have aIdx = 10 and there are only 2 body parts, 7 empty
    body parts will be created to fill slots 2 through 8.  If you
    just want to add a body part at the end, use AddBodyPart().
    */
  virtual void setBodyPart(int aIdx, const KMMessagePart* aPart);
    
  /** Append a body part to the message. */
  virtual void addBodyPart(const KMMessagePart* aPart);

  /** Delete all body parts. */
  virtual void deleteBodyParts(void);

  /** Open a window containing the complete, unparsed, message. */
  virtual void viewSource(const QString windowCaption) const;

  /** Strip email address from string. Examples:
   * "Stefan Taferner <taferner@kde.org>" returns "Stefan Taferner"
   * "joe@nowhere.com" returns "joe@nowhere.com" */
  static const QString stripEmailAddr(const QString emailAddr);

  /** Converts given email address to a nice HTML mailto: anchor. 
   * If stripped is TRUE then the visible part of the anchor contains
   * only the name part and not the given emailAddr. */
  static const QString emailAddrAsAnchor(const QString emailAddr, 
					 bool stripped=TRUE);

protected:
  DwMessage* mMsg;
  bool       mNeedsAssembly;
};

typedef KMMessage* KMMessagePtr;

#endif /*kmmessage_h*/
