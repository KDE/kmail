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
protected:
  KMMessage(KMFolder*, DwMessage* = NULL);

public:
  typedef enum {
    stUnknown=' ', stNew='N', stUnread='U', stOld='O', stDeleted='D' 
  } Status; // see below for a conversion function to strings

  KMMessage();
  virtual ~KMMessage();

  Status status(void) const { return mStatus; }

  /** Set the status. Ensures that index in the folder is also updated */
  virtual void setStatus(Status);

  void del(void) { };

  /** Return the BasicMessage contents as a string */
  const char* asString(void);

  /**
   Set fields that are either automatically set (Message-id)
   or that do not change from one message to another (MIME-Version).
   Call this method before sending *after* all changes to the message
   are done because this method does things different if there are
   attachments / multiple body parts.
   */
  virtual void setAutomaticFields();
    
  /** Get or set the 'Date' header field */
  const char* dateStr(void) const;
  time_t date(void) const;
  void setDate(time_t aUnixTime);

  /** Get or set the 'To' header field */
  const char* to(void) const;
  void setTo(const char* aStr);

  /** Get or set the 'ReplyTo' header field */
  const char* replyTo(void) const;
  void setReplyTo(const char* aStr);
  void setReplyTo(KMMessage*);

  /** Get or set the 'Cc' header field */
  const char* cc(void) const;
  void setCc(const char* aStr);

  /** Get or set the 'Bcc' header field */
  const char* bcc(void) const;
  void setBcc(const char* aStr);

  /** Get or set the 'From' header field */
  const char* from(void) const;
  void setFrom(const char* aStr);

  /** Get or set the 'Subject' header field */
  const char* subject(void) const;
  void setSubject(const char* aStr);

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
  const char* body(long* length_return=NULL) const;
  void setBody(const char* aStr);

  /** Number of body parts the message has. This is one for plain messages
      without any attachment. */
  int numBodyParts(void) const;

  /** Get the body part at position in aIdx.  Indexing starts at 0.
    If there is no body part at that index, aPart will have its
    attributes set to empty values. */
  void bodyPart(int aIdx, KMMessagePart* aPart);
    
  /** Set the body part at position in aIdx.  Indexing starts at 0.
    If you have aIdx = 10 and there are only 2 body parts, 7 empty
    body parts will be created to fill slots 2 through 8.  If you
    just want to add a body part at the end, use AddBodyPart().
    */
  void setBodyPart(int aIdx, const KMMessagePart* aPart);
    
  /** Append a body part to the message. */
  void addBodyPart(const KMMessagePart* aPart);

  /** Convert message status to string. */
  static const char* statusToStr(Status);

  //----| some temporary methods |-----------------------------------

  // create a reply to this message
  KMMessage* reply(void) { return new KMMessage; }


protected:
  friend class KMFolder;

  void setOwner(KMFolder*);
  DwString& msgStr(void) { return mMsgStr; }
  virtual void takeMessage(DwMessage* aMsg);

  DwMessage* mMsg;
  DwString   mMsgStr;
  KMFolder*  mOwner;
  Status     mStatus;
};


#endif /*kmmessage_h*/
