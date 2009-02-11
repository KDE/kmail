// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
#ifndef MAILINGLIST_MAGIC_H
#define MAILINGLIST_MAGIC_H

#include <kurl.h>

#include <QString>
#include <QByteArray>

class KMMessage;
class KConfigGroup;

namespace KMail
{

/**
 * Class is used for all Mailing List handling inside
 * KMail. The static detect method is used to detect
 * a full set of ml information from a message. The
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
  static QString name( const KMMessage  *message, QByteArray &header_name,
                       QString &header_value );
public:
  MailingList();

  int features() const;

  void setHandler( Handler han );
  Handler handler() const;

  void setPostURLS ( const KUrl::List& );
  KUrl::List postURLS() const;

  void setSubscribeURLS( const KUrl::List& );
  KUrl::List subscribeURLS() const;

  void setUnsubscribeURLS ( const KUrl::List& );
  KUrl::List unsubscribeURLS() const;

  void setHelpURLS( const KUrl::List& );
  KUrl::List helpURLS() const;

  void setArchiveURLS( const KUrl::List& );
  KUrl::List archiveURLS() const;

  void setId( const QString& );
  QString id() const;

  void writeConfig( KConfigGroup & config ) const;
  void readConfig( KConfigGroup & config );
private:
  int        mFeatures;
  Handler    mHandler;
  KUrl::List mPostURLS;
  KUrl::List mSubscribeURLS;
  KUrl::List mUnsubscribeURLS;
  KUrl::List mHelpURLS;
  KUrl::List mArchiveURLS;
  QString    mId;
};

}

#endif
