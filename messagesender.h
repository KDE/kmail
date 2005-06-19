#ifndef __KMAIL_MESSAGESENDER_H__
#define __KMAIL_MESSAGESENDER_H__

class KMMessage;

namespace KMail {

class MessageSender {
protected:
  virtual ~MessageSender() = 0;

public:
  /** Send given message. The message is either queued or sent
    immediately. The default behaviour, as selected with
    setSendImmediate(), can be overwritten with the parameter
    sendNow (by specifying TRUE or FALSE).
    The sender takes ownership of the given message on success,
    so DO NOT DELETE OR MODIFY the message further.
    Returns TRUE on success. */
  bool send( KMMessage * msg ) { return doSend( msg, -1 ); }
  bool send( KMMessage* msg, bool sendNow ) { return doSend( msg, sendNow ); }

  /** Start sending all queued messages. Returns TRUE on success. */
  virtual bool sendQueued() = 0;

  virtual void readConfig() = 0;
  virtual void writeConfig( bool withSync = true ) = 0;

  virtual bool sendImmediate() const = 0;
  virtual void setSendImmediate( bool immediate ) = 0;

  virtual bool sendQuotedPrintable() const = 0;
  virtual void setSendQuotedPrintable( bool qp ) = 0;
protected:
  virtual bool doSend( KMMessage * msg, short sendNow ) = 0;
};

inline MessageSender::~MessageSender() {}

}

#endif /* __KMAIL_MESSAGESENDER_H__ */

