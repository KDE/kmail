// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
#ifndef MAILINGLIST_MAGIC_H
#define MAILINGLIST_MAGIC_H

#include <kurl.h>
#include <qstring.h>

class KMMessage;
class KConfig;

namespace KMail
{

/**
 * Class is used for all Mailing List handling inside
 * KMail. The static detect method is used to detect
 * a full set of ml informations from a message. The
 * features() method defines which addresses the mailing
 * has defined.
 *
 * @author Zack Rusin <zack@kde.org>
 */
class MailingList
{
public:
  enum Handler {
    KMail,
    Browser
  };

  enum Supports {
    None         = 0 << 0,
    Post         = 1 << 0,
    Subscribe    = 1 << 1,
    Unsubscribe  = 1 << 2,
    Help         = 1 << 3,
    Archive      = 1 << 4,
    Id           = 1 << 5
  };
public:
  static MailingList detect( const KMMessage* msg );
  static QString name( const KMMessage  *message, QCString &header_name,
		       QString &header_value );
public:
  MailingList();

  int features() const;

  void setHandler( Handler han );
  Handler handler() const;

  void setPostURLS ( const KURL::List& );
  KURL::List postURLS() const;

  void setSubscribeURLS( const KURL::List& );
  KURL::List subscribeURLS() const;

  void setUnsubscribeURLS ( const KURL::List& );
  KURL::List unsubscribeURLS() const;

  void setHelpURLS( const KURL::List& );
  KURL::List helpURLS() const;

  void setArchiveURLS( const KURL::List& );
  KURL::List archiveURLS() const;

  void setId( const QString& );
  QString id() const;

  void writeConfig( KConfig* config ) const;
  void readConfig( KConfig* config );
private:
  int        mFeatures;
  Handler    mHandler;
  KURL::List mPostURLS;
  KURL::List mSubscribeURLS;
  KURL::List mUnsubscribeURLS;
  KURL::List mHelpURLS;
  KURL::List mArchiveURLS;
  QString    mId;
};

}

#endif
