/* kmmessage.h: Mime Message Class
 *
 */
#ifndef kmmessage_h
#define kmmessage_h

#include <mimelib/string.h>

#include <time.h>

class KMFolder;
class DwMessage;
class KMMessagePart;

class KMMessage
{
  friend class KMFolder;

protected:
  KMMessage(KMFolder*, DwMessage* = NULL);

public:
  typedef enum {
    stUnknown=' ', stNew='N', stUnread='U', stOld='O', stDeleted='D',
    stReplied='A'
  } Status; // see below for a conversion function to strings

  KMMessage();
  virtual ~KMMessage();

  /** Returns the status of the message */
  Status status(void) const { return mStatus; }

  /** Set the status. Ensures that index in the folder is also updated. */
  virtual void setStatus(Status);

  /** Convert the given message status to a string. */
  static const char* statusToStr(Status);

  /** Mark the message as deleted */
  void del(void) { setStatus(stDeleted); }

  /** Undelete the message. Same as touch */
  void undel(void) { setStatus(stOld); }

  /** Touch the message - mark it as read */
  void touch(void) { setStatus(stOld); }

  /** Create a reply to this message, filling all required header fields
   with the proper values. The returned message is not associated with
   any folder. */
  virtual KMMessage* reply(void);

  /** Return the message contents as a string */
  virtual const char* asString(void);

  /** Set fields that are either automatically set (Message-id)
   or that do not change from one message to another (MIME-Version).
   Call this method before sending *after* all changes to the message
   are done because this method does things different if there are
   attachments / multiple body parts. */
  virtual void setAutomaticFields(void);
    
  /** Get or set the 'Date' header field */
  virtual const char* dateStr(void) const;
  virtual time_t date(void) const;
  virtual void setDate(time_t aUnixTime);

  /** Get or set the 'To' header field */
  virtual const char* to(void) const;
  virtual void setTo(const char* aStr);

  /** Get or set the 'ReplyTo' header field */
  virtual const char* replyTo(void) const;
  virtual void setReplyTo(const char* aStr);
  virtual void setReplyTo(KMMessage*);

  /** Get or set the 'Cc' header field */
  virtual const char* cc(void) const;
  virtual void setCc(const char* aStr);

  /** Get or set the 'Bcc' header field */
  virtual const char* bcc(void) const;
  virtual void setBcc(const char* aStr);

  /** Get or set the 'From' header field */
  virtual const char* from(void) const;
  virtual void setFrom(const char* aStr);

  /** Get or set the 'Subject' header field */
  virtual const char* subject(void) const;
  virtual void setSubject(const char* aStr);

  /** Get or set the 'Content-Type' header field
   The member functions that involve enumerated types (ints)
   will work only for well-known types or subtypes. */
  virtual const char* typeStr(void) const;
  virtual int type(void) const;
  virtual void setTypeStr(const char* aStr);
  virtual void setType(int aType);
  // Subtype
  virtual const char* subtypeStr(void) const;
  virtual int subtype(void) const;
  virtual void setSubtypeStr(const char* aStr);
  virtual void setSubtype(int aSubtype);

  /** Get or set the 'Content-Transfer-Encoding' header field
    The member functions that involve enumerated types (ints)
    will work only for well-known encodings. */
  virtual const char* contentTransferEncodingStr(void) const;
  virtual int  contentTransferEncoding(void) const;
  virtual void setContentTransferEncodingStr(const char* aStr);
  virtual void setContentTransferEncoding(int aCte);

  /** Cte is short for ContentTransferEncoding.
      These functions are an alternative to the ones with longer names. */
  const char* cteStr(void) const { return contentTransferEncodingStr(); }
  int cte(void) const { return contentTransferEncoding(); }
  void setCteStr(const char* aStr) { setContentTransferEncodingStr(aStr); }
  void setCte(int aCte) { setContentTransferEncoding(aCte); }

  /** Get or set the message body */
  virtual const char* body(long* length_return=NULL) const;
  virtual void setBody(const char* aStr);

  /** Number of body parts the message has. This is one for plain messages
      without any attachment. */
  virtual int numBodyParts(void) const;

  /** Get the body part at position in aIdx.  Indexing starts at 0.
    If there is no body part at that index, aPart will have its
    attributes set to empty values. */
  virtual void bodyPart(int aIdx, KMMessagePart* aPart);
    
  /** Set the body part at position in aIdx.  Indexing starts at 0.
    If you have aIdx = 10 and there are only 2 body parts, 7 empty
    body parts will be created to fill slots 2 through 8.  If you
    just want to add a body part at the end, use AddBodyPart().
    */
  virtual void setBodyPart(int aIdx, const KMMessagePart* aPart);
    
  /** Append a body part to the message. */
  virtual void addBodyPart(const KMMessagePart* aPart);

  /** Owning folder or NULL if none. */
  KMFolder* owner(void) const { return mOwner; }

protected:
  void setOwner(KMFolder*);
  DwString& msgStr(void) { return mMsgStr; }
  virtual void takeMessage(DwMessage* aMsg);

  DwMessage* mMsg;
  DwString   mMsgStr;
  KMFolder*  mOwner;
  Status     mStatus;
};


#endif /*kmmessage_h*/
