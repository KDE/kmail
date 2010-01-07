// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
#ifndef MAILINGLIST_MAGIC_H
#define MAILINGLIST_MAGIC_H

#include <kurl.h>

#include <QString>
#include <QByteArray>

#include <kmime/kmime_message.h>

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
 * Mailing list header fields, List-*, are defined in RFC2369 and List-ID
 * is defined in RFC2919.
 * Archive-At list header field is defined in RFC5064
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

  /**
    Defines the bitfields that compared with features(), returns their existence
  */
  enum Supports {
    None         = 0 << 0, /** no mailinglist fields exist */
    Post         = 1 << 0, /** List-Post header exists */
    Subscribe    = 1 << 1, /** List-Subscribe header exists */
    Unsubscribe  = 1 << 2, /** List-Unsubscribe header exists */
    Help         = 1 << 3, /** List-Help header exists */
    Archive      = 1 << 4, /** List-Archive header exists */
    Id           = 1 << 5, /** List-ID header exists */
    Owner        = 1 << 6, /** List-Owner header exists */
    ArchivedAt    = 1 << 7  /** Archive-At header exists */
  };
public:
  static MailingList detect(  const KMime::Message::Ptr &msg );
  static QString name( const KMime::Message::Ptr &message, QByteArray &header_name,
                       QString &header_value );
public:
  /**
   * Empty Contructor
   */
  MailingList();

  /**
   * Return an integer bitmap to be bitwise and-ed with the Supports enum
   * to determine which features were present
   */
  int features() const;

  /**
   * Set the handler for this object. The handler is just stored here.
   */
  void setHandler( Handler han );
  Handler handler() const;

  /**
   * Sets/returns the list of URLs for List-Post
   */
  void setPostURLS ( const KUrl::List& );
  KUrl::List postURLS() const;

  /**
   * Sets/returns the list of URLs for List-Subscribe
   */
  void setSubscribeURLS( const KUrl::List& );
  KUrl::List subscribeURLS() const;

  /**
   * Sets/returns the list of URLs for List-Unsubscribe
   */
  void setUnsubscribeURLS ( const KUrl::List& );
  KUrl::List unsubscribeURLS() const;

  /**
   * Sets/returns the list of URLs for List-Help
   */
  void setHelpURLS( const KUrl::List& );
  KUrl::List helpURLS() const;

  /**
   * Sets/returns the list of URLs for List-Archive
   */
  void setArchiveURLS( const KUrl::List& );
  KUrl::List archiveURLS() const;

  /**
   * Sets/returns the list of URLs for List-Owner
   */
  void setOwnerURLS( const KUrl::List& );
  KUrl::List ownerURLS() const;

  /**
   * Sets/returns the list of URLs for Archived-At
   */
  void setArchivedAt( const QString& );
  QString archivedAt() const;

  /**
   * Sets/returns the list of URLs for List-ID
   */
  void setId( const QString& );
  QString id() const;

  /**
   * Saves the configuration for this mailing list in the config arguement
   */
  void writeConfig( KConfigGroup & config ) const;

  /**
   * Restores the configuration for this mailing list from the config arguement
   */
  void readConfig( KConfigGroup & config );
private:
  int        mFeatures;
  Handler    mHandler;
  KUrl::List mPostURLS;
  KUrl::List mSubscribeURLS;
  KUrl::List mUnsubscribeURLS;
  KUrl::List mHelpURLS;
  KUrl::List mArchiveURLS;
  KUrl::List mOwnerURLS;
  QString    mArchivedAt;
  QString    mId;
};

}

#endif
