#ifndef MAILCOMPOSERIFACE_H
#define MAILCOMPOSERIFACE_H

#include <dcopobject.h>
#include <kurl.h>

/**
  DCOP interface for mail composer window. The address header fields are set,
  when the composer is constructed. KMailIface::openComposer() returns a
  reference to the DCOP interface of the new composer window, which provides the
  functions defined in the MailComposerIface.
*/
class MailComposerIface : virtual public DCOPObject
{
    K_DCOP
  k_dcop:
    /**
      Send message.      

      @param how 0 for deafult method, 1 for sending now, 2 for sending later.
    */
    virtual void send(int how) = 0;
    /**
      Add url as attachment with a user-defined comment.
    */
    virtual void addAttachment(KURL url,QString comment) = 0;
    /**
      Set message body.
    */
    virtual void setBody (QString body) = 0;
    /*
      Add attachment.

      @param name Name of Attachment
      @param cte Content Transfer Encoding
      @param data Data to be attached
      @param type MIME content type
      @param subType MIME content sub type
      @param paramAttr Attribute name of parameter of content type
      @param paramValue Value of parameter of content type
      @param contDisp Content disposition
    */
    virtual void addAttachment(const QString &name,
                              const QCString &cte,
                              const QByteArray &data,
                              const QCString &type,
                              const QCString &subType,
                              const QCString &paramAttr,
                              const QString &paramValue,
                              const QCString &contDisp) = 0;
};

#endif
