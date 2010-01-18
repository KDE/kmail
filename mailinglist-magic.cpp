// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-

#include "mailinglist-magic.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kurl.h>
#include <kdebug.h>

#include <QStringList>
#include <QByteArray>

#include <boost/shared_ptr.hpp>

using namespace KMail;

typedef QString (*MagicDetectorFunc) (const KMime::Message::Ptr &, QByteArray &, QString &);

/* Sender: (owner-([^@]+)|([^@+]-owner)@ */
static QString check_sender( const KMime::Message::Ptr &message,
                             QByteArray &header_name,
                             QString &header_value )
{
  QString header = message->sender()->asUnicodeString();

  if ( header.isEmpty() )
    return QString();

  if ( header.left( 6 ) == "owner-" )
  {
    header_name = "Sender";
    header_value = header;
    header = header.mid( 6, header.indexOf( '@' ) - 6 );

  } else {
    int index = header.indexOf( "-owner@ " );
    if ( index == -1 )
      return QString();

    header.truncate( index );
    header_name = "Sender";
    header_value = header;
  }

  return header;
}

/* X-BeenThere: ([^@]+) */
static QString check_x_beenthere( const KMime::Message::Ptr &message,
                                  QByteArray &header_name,
                                  QString &header_value )
{
  QString header = message->headerByType( "X-BeenThere" ) ?message->headerByType( "X-BeenThere" )->asUnicodeString() : "";
  if ( header.isNull() || header.indexOf( '@' ) == -1 )
    return QString();

  header_name = "X-BeenThere";
  header_value = header;
  header.truncate( header.indexOf( '@' ) );
  return header;
}

/* Delivered-To:: <([^@]+) */
static QString check_delivered_to( const KMime::Message::Ptr &message,
                                   QByteArray &header_name,
                                   QString &header_value )
{
   QString header = message->headerByType( "Delivered-To" ) ?message->headerByType( "Delivered-To" )->asUnicodeString() : "";
 if ( header.isNull() || header.left(13 ) != "mailing list"
       || header.indexOf( '@' ) == -1 )
    return QString();

  header_name = "Delivered-To";
  header_value = header;

  return header.mid( 13, header.indexOf( '@' ) - 13 );
}

/* X-Mailing-List: <?([^@]+) */
static QString check_x_mailing_list( const KMime::Message::Ptr &message,
                                     QByteArray &header_name,
                                     QString &header_value )
{
  QString header = message->headerByType( "X-Mailing-List" ) ?message->headerByType( "X-Mailing-List" )->asUnicodeString() : "";
  if ( header.isEmpty() )
    return QString();

  if ( header.indexOf( '@' ) < 1 )
    return QString();

  header_name = "X-Mailing-List";
  header_value = header;
  if ( header[0] == '<' )
    header = header.mid(1,  header.indexOf( '@' ) - 1);
  else
    header.truncate( header.indexOf( '@' ) );
  return header;
}

/* List-Id: [^<]* <([^.]+) */
static QString check_list_id( const KMime::Message::Ptr &message,
                              QByteArray &header_name,
                              QString &header_value )
{
  int lAnglePos, firstDotPos;
  QString header = message->headerByType( "List-Id" ) ?message->headerByType( "List-Id" )->asUnicodeString() : "";
  if ( header.isEmpty() )
    return QString();

  lAnglePos = header.indexOf( '<' );
  if ( lAnglePos < 0 )
    return QString();

  firstDotPos = header.indexOf( '.', lAnglePos );
  if ( firstDotPos < 0 )
    return QString();

  header_name = "List-Id";
  header_value = header.mid( lAnglePos );
  header = header.mid( lAnglePos + 1, firstDotPos - lAnglePos - 1 );
  return header;
}


/* List-Post: <mailto:[^< ]*>) */
static QString check_list_post( const KMime::Message::Ptr &message,
                                QByteArray &header_name,
                                QString &header_value )
{
  QString header = message->headerByType( "List-Post" ) ?message->headerByType( "List-Post" )->asUnicodeString() : "";
  if ( header.isEmpty() )
    return QString();

  int lAnglePos = header.indexOf( "<mailto:" );
  if ( lAnglePos < 0 )
    return QString();

  header_name = "List-Post";
  header_value = header;
  header = header.mid( lAnglePos + 8, header.length());
  header.truncate( header.indexOf('@') );
  return header;
}

/* Mailing-List: list ([^@]+) */
static QString check_mailing_list( const KMime::Message::Ptr &message,
                                   QByteArray &header_name,
                                   QString &header_value )
{
  QString header = message->headerByType( "Mailing-List" ) ?message->headerByType( "Mailing-List" )->asUnicodeString() : "";
  if ( header.isEmpty() )
    return QString();

  if (header.left( 5 ) != "list " || header.indexOf( '@' ) < 5 )
    return QString();

  header_name = "Mailing-List";
  header_value = header;
  header = header.mid(5,  header.indexOf( '@' ) - 5);
  return header;
}


/* X-Loop: ([^@]+) */
static QString check_x_loop( const KMime::Message::Ptr &message,
                             QByteArray &header_name,
                             QString &header_value )
{
  QString header = message->headerByType( "X-Loop" ) ?message->headerByType( "X-Loop" )->asUnicodeString() : "";
  if ( header.isEmpty() )
    return QString();

  if (header.indexOf( '@' ) < 2 )
    return QString();

  header_name = "X-Loop";
  header_value = header;
  header.truncate(header.indexOf( '@' ));
  return header;
}

/* X-ML-Name: (.+) */
static QString check_x_ml_name( const KMime::Message::Ptr &message,
                                QByteArray &header_name,
                                QString &header_value )
{
  QString header = message->headerByType( "X-ML-Name" ) ?message->headerByType( "X-ML-Name" )->asUnicodeString() : "";
  if ( header.isEmpty() )
    return QString();

  header_name = "X-ML-Name";
  header_value = header;
  header.truncate(header.indexOf( '@' ));
  return header;
}

MagicDetectorFunc magic_detector[] =
{
  check_list_id,
  check_list_post,
  check_sender,
  check_x_mailing_list,
  check_mailing_list,
  check_delivered_to,
  check_x_beenthere,
  check_x_loop,
  check_x_ml_name
};

static const int num_detectors = sizeof (magic_detector) / sizeof (magic_detector[0]);

static QStringList
headerToAddress( const QString& header )
{
  QStringList addr;
  int start = 0;
  int end = 0;

  if ( header.isEmpty() )
    return addr;

  while ( (start = header.indexOf( "<", start )) != -1 ) {
    if ( (end = header.indexOf( ">", ++start ) ) == -1 ) {
      kDebug()<<"Serious mailing list header parsing error !";
      return addr;
    }
    //kDebug()<<"Mailing list ="<<header.mid( start, end - start );
    addr.append( header.mid( start, end - start ) );
  }
  return  addr;
}

MailingList
MailingList::detect( const KMime::Message::Ptr &message )
{
  MailingList mlist;

  mlist.setPostURLS( headerToAddress(
                       message->headerByType( "List-Post" ) ? message->headerByType( "List-Post" )->asUnicodeString() : "" ) );
  mlist.setHelpURLS( headerToAddress(
                       message->headerByType( "List-Help" ) ? message->headerByType( "List-Help" )->asUnicodeString() : "" ) );
  mlist.setSubscribeURLS( headerToAddress(
                          message->headerByType( "List-Subscribe" ) ? message->headerByType( "List-Subscribe" )->asUnicodeString() : ""));
  mlist.setUnsubscribeURLS( headerToAddress(
                          message->headerByType( "List-Unsubscribe" ) ? message->headerByType( "List-Unsubscribe" )->asUnicodeString() : ""));
  mlist.setArchiveURLS( headerToAddress(
                          message->headerByType( "List-Arhive" ) ? message->headerByType( "List-Archive" )->asUnicodeString() : "" ) );
  mlist.setOwnerURLS( headerToAddress(
                          message->headerByType( "List-Owner" ) ? message->headerByType( "List-Owner" )->asUnicodeString() : "" ) );
  mlist.setArchivedAt( message->headerByType( "Archived-At" ) ? message->headerByType( "Archived-At" )->asUnicodeString() : "" );
  mlist.setId( message->headerByType( "List-Id" ) ? message->headerByType( "List-Id" )->asUnicodeString() : ""  );

  return mlist;
}

QString
MailingList::name( const KMime::Message::Ptr &message, QByteArray &header_name,
                   QString &header_value )
{
  QString mlist;
  header_name = QByteArray();
  header_value.clear();

  if ( !message )
    return QString();

  for (int i = 0; i < num_detectors; i++) {
    mlist = magic_detector[i] (message, header_name, header_value);
    if ( !mlist.isNull() )
      return mlist;
  }

  return QString();
}

MailingList::MailingList()
  : mFeatures( None ), mHandler( KMail )
{
}

int MailingList::features() const
{
  return mFeatures;
}

void MailingList::setHandler( MailingList::Handler han )
{
  mHandler = han;
}

MailingList::Handler MailingList::handler() const
{
  return mHandler;
}

void MailingList::setPostURLS ( const KUrl::List& lst )
{
  mFeatures |= Post;
  if ( lst.empty() ) {
    mFeatures ^= Post;
  }
  mPostURLS = lst;
}

KUrl::List MailingList::postURLS() const
{
  return mPostURLS;
}

void MailingList::setSubscribeURLS( const KUrl::List& lst )
{
  mFeatures |= Subscribe;
  if ( lst.empty() ) {
    mFeatures ^= Subscribe;
  }

  mSubscribeURLS = lst;
}

KUrl::List MailingList::subscribeURLS() const
{
  return mSubscribeURLS;
}

void MailingList::setUnsubscribeURLS( const KUrl::List& lst )
{
  mFeatures |= Unsubscribe;
  if ( lst.empty() ) {
    mFeatures ^= Unsubscribe;
  }

  mUnsubscribeURLS = lst;
}

KUrl::List MailingList::unsubscribeURLS() const
{
  return mUnsubscribeURLS;
}

void MailingList::setHelpURLS( const KUrl::List& lst )
{
  mFeatures |= Help;
  if ( lst.empty() ) {
    mFeatures ^= Help;
  }

  mHelpURLS = lst;
}

KUrl::List MailingList::helpURLS() const
{
  return mHelpURLS;
}

void MailingList::setArchiveURLS( const KUrl::List& lst )
{
  mFeatures |= Archive;
  if ( lst.empty() ) {
    mFeatures ^= Archive;
  }

  mArchiveURLS = lst;
}

KUrl::List MailingList::archiveURLS() const
{
  return mArchiveURLS;
}

void MailingList::setOwnerURLS( const KUrl::List& lst )
{
  mFeatures |= Owner;
  if ( lst.empty() ) {
    mFeatures ^= Owner;
  }

  mOwnerURLS = lst;
}

KUrl::List MailingList::ownerURLS() const
{
  return mOwnerURLS;
}

void MailingList::setArchivedAt( const QString& str )
{
  mFeatures |= ArchivedAt;
  if ( str.isEmpty() ) {
    mFeatures ^= ArchivedAt;
  }

  mArchivedAt = str;
}
QString MailingList::archivedAt() const
{
  return mArchivedAt;
}

void MailingList::setId( const QString& str )
{
  mFeatures |= Id;
  if ( str.isEmpty() ) {
    mFeatures ^= Id;
  }

  mId = str;
}

QString MailingList::id() const
{
  return mId;
}

void MailingList::writeConfig( KConfigGroup & config ) const
{
  config.writeEntry( "MailingListFeatures", mFeatures );
  config.writeEntry( "MailingListHandler", (int)mHandler );
  config.writeEntry( "MailingListId",  mId );
  config.writeEntry( "MailingListPostingAddress", mPostURLS.toStringList() );
  config.writeEntry( "MailingListSubscribeAddress", mSubscribeURLS.toStringList() );
  config.writeEntry( "MailingListUnsubscribeAddress", mUnsubscribeURLS.toStringList() );
  config.writeEntry( "MailingListArchiveAddress", mArchiveURLS.toStringList() );
  config.writeEntry( "MailingListOwnerAddress", mOwnerURLS.toStringList() );
  config.writeEntry( "MailingListHelpAddress", mHelpURLS.toStringList() );
  /* Note: mArchivedAt deliberately not saved here as it refers to a single 
   * instance of a message rather than an element of a general mailing list.
   * http://reviewboard.kde.org/r/1768/#review2783
   */
}

void MailingList::readConfig( KConfigGroup & config )
{
  mFeatures =  config.readEntry( "MailingListFeatures", 0 );
  mHandler = static_cast<MailingList::Handler>(
    config.readEntry( "MailingListHandler", (int)MailingList::KMail ) );

  mId = config.readEntry("MailingListId");
  mPostURLS        = config.readEntry( "MailingListPostingAddress", QStringList() );
  mSubscribeURLS   = config.readEntry( "MailingListSubscribeAddress", QStringList() );
  mUnsubscribeURLS = config.readEntry( "MailingListUnsubscribeAddress", QStringList() );
  mArchiveURLS     = config.readEntry( "MailingListArchiveAddress", QStringList() );
  mOwnerURLS       = config.readEntry( "MailingListOwnerddress", QStringList() );
  mHelpURLS        = config.readEntry( "MailingListHelpAddress", QStringList() );
}
