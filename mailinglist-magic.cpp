// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mailinglist-magic.h"

#include "kmmessage.h"

#include <kconfig.h>
#include <kurl.h>
#include <kdebug.h>

#include <qstringlist.h>

using namespace KMail;

typedef QString (*MagicDetectorFunc) (const KMMessage *, QCString &, QString &);

/* Sender: (owner-([^@]+)|([^@+]-owner)@ */
static QString check_sender(const KMMessage  *message,
                            QCString &header_name,
                            QString &header_value )
{
  QString header = message->headerField( "Sender" );

  if ( header.isEmpty() )
    return QString::null;

  if ( header.left( 6 ) == "owner-" )
  {
    header_name = "Sender";
    header_value = header;
    header = header.mid( 6, header.find( '@' ) - 6 );

  } else {
    int index = header.find( "-owner@ " );
    if ( index == -1 )
      return QString::null;

    header.truncate( index );
    header_name = "Sender";
    header_value = header;
  }

  return header;
}

/* X-BeenThere: ([^@]+) */
static QString check_x_beenthere(const KMMessage  *message,
                                 QCString &header_name,
                                 QString &header_value )
{
  QString header = message->headerField( "X-BeenThere" );
  if ( header.isNull() || header.find( '@' ) == -1 )
    return QString::null;

  header_name = "X-BeenThere";
  header_value = header;
  header.truncate( header.find( '@' ) );
  return header;
}

/* Delivered-To:: <([^@]+) */
static QString check_delivered_to(const KMMessage  *message,
                                  QCString &header_name,
                                  QString &header_value )
{
  QString header = message->headerField( "Delivered-To" );
  if ( header.isNull() || header.left(13 ) != "mailing list"
       || header.find( '@' ) == -1 )
    return QString::null;

  header_name = "Delivered-To";
  header_value = header;

  return header.mid( 13, header.find( '@' ) - 13 );
}

/* X-Mailing-List: <?([^@]+) */
static QString check_x_mailing_list(const KMMessage  *message,
                                    QCString &header_name,
                                    QString &header_value )
{
  QString header = message->headerField( "X-Mailing-List");
  if ( header.isEmpty() )
    return QString::null;

  if ( header.find( '@' ) < 1 )
    return QString::null;

  header_name = "X-Mailing-List";
  header_value = header;
  if ( header[0] == '<' )
    header = header.mid(1,  header.find( '@' ) - 1);
  else
    header.truncate( header.find( '@' ) );
  return header;
}

/* List-Id: [^<]* <([^.]+) */
static QString check_list_id(const KMMessage  *message,
			     QCString &header_name,
			     QString &header_value )
{
  int lAnglePos, firstDotPos;
  QString header = message->headerField( "List-Id" );
  if ( header.isEmpty() )
    return QString::null;

  lAnglePos = header.find( '<' );
  if ( lAnglePos < 0 )
    return QString::null;

  firstDotPos = header.find( '.', lAnglePos );
  if ( firstDotPos < 0 )
    return QString::null;

  header_name = "List-Id";
  header_value = header.mid( lAnglePos );
  header = header.mid( lAnglePos + 1, firstDotPos - lAnglePos - 1 );
  return header;
}


/* List-Post: <mailto:[^< ]*>) */
static QString check_list_post(const KMMessage  *message,
                               QCString &header_name,
                               QString &header_value )
{
  QString header = message->headerField( "List-Post" );
  if ( header.isEmpty() )
    return QString::null;

  int lAnglePos = header.find( "<mailto:" );
  if ( lAnglePos < 0 )
    return QString::null;

  header_name = "List-Post";
  header_value = header;
  header = header.mid( lAnglePos + 8, header.length());
  header.truncate( header.find('@') );
  return header;
}

/* Mailing-List: list ([^@]+) */
static QString check_mailing_list(const KMMessage  *message,
                                  QCString &header_name,
                                  QString &header_value )
{
  QString header = message->headerField( "Mailing-List");
  if ( header.isEmpty() )
    return QString::null;

  if (header.left( 5 ) != "list " || header.find( '@' ) < 5 )
    return QString::null;

  header_name = "Mailing-List";
  header_value = header;
  header = header.mid(5,  header.find( '@' ) - 5);
  return header;
}


/* X-Loop: ([^@]+) */
static QString check_x_loop(const KMMessage  *message,
                            QCString &header_name,
                            QString &header_value ){
  QString header = message->headerField( "X-Loop");
  if ( header.isEmpty() )
    return QString::null;

  if (header.find( '@' ) < 2 )
    return QString::null;

  header_name = "X-Loop";
  header_value = header;
  header.truncate(header.find( '@' ));
  return header;
}

/* X-ML-Name: (.+) */
static QString check_x_ml_name(const KMMessage  *message,
                               QCString &header_name,
                               QString &header_value ){
  QString header = message->headerField( "X-ML-Name");
  if ( header.isEmpty() )
    return QString::null;

  header_name = "X-ML-Name";
  header_value = header;
  header.truncate(header.find( '@' ));
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

  while ( (start = header.find( "<", start )) != -1 ) {
    if ( (end = header.find( ">", ++start ) ) == -1 ) {
      kdDebug(5006)<<k_funcinfo<<"Serious mailing list header parsing error !"<<endl;
      return addr;
    }
    kdDebug(5006)<<"Mailing list = "<<header.mid( start, end - start )<<endl;
    addr.append( header.mid( start, end - start ) );
  }
  return  addr;
}

MailingList
MailingList::detect( const KMMessage *message )
{
  MailingList mlist;

  mlist.setPostURLS( headerToAddress(
                       message->headerField( "List-Post" ) ) );
  mlist.setHelpURLS( headerToAddress(
                       message->headerField( "List-Help" ) ) );
  mlist.setSubscribeURLS( headerToAddress(
                            message->headerField( "List-Subscribe" ) ) );
  mlist.setUnsubscribeURLS( headerToAddress(
                              message->headerField( "List-Unsubscribe" ) ) );
  mlist.setArchiveURLS( headerToAddress(
                          message->headerField( "List-Archive" ) ) );
  mlist.setId( message->headerField( "List-Id" ) );

  return mlist;
}

QString
MailingList::name( const KMMessage  *message, QCString &header_name,
                   QString &header_value )
{
  QString mlist;
  header_name = QCString();
  header_value = QString::null;

  if ( !message )
    return QString::null;

  for (int i = 0; i < num_detectors; i++) {
    mlist = magic_detector[i] (message, header_name, header_value);
    if ( !mlist.isNull() )
      return mlist;
  }

  return QString::null;
}

MailingList::MailingList()
  : mFeatures( None ), mHandler( KMail )
{
}

int
MailingList::features() const
{
  return mFeatures;
}

void
MailingList::setHandler( MailingList::Handler han )
{
  mHandler = han;
}
MailingList::Handler
MailingList::handler() const
{
  return mHandler;
}

void
MailingList::setPostURLS ( const KURL::List& lst )
{
  mFeatures |= Post;
  if ( lst.empty() ) {
    mFeatures ^= Post;
  }
  mPostURLS = lst;
}
KURL::List
MailingList::postURLS() const
{
  return mPostURLS;
}

void
MailingList::setSubscribeURLS( const KURL::List& lst )
{
  mFeatures |= Subscribe;
  if ( lst.empty() ) {
    mFeatures ^= Subscribe;
  }

  mSubscribeURLS = lst;
}
KURL::List
MailingList::subscribeURLS() const
{
  return mSubscribeURLS;
}

void
MailingList::setUnsubscribeURLS( const KURL::List& lst )
{
  mFeatures |= Unsubscribe;
  if ( lst.empty() ) {
    mFeatures ^= Unsubscribe;
  }

  mUnsubscribeURLS = lst;
}
KURL::List MailingList::unsubscribeURLS() const
{
  return mUnsubscribeURLS;
}

void
MailingList::setHelpURLS( const KURL::List& lst )
{
  mFeatures |= Help;
  if ( lst.empty() ) {
    mFeatures ^= Help;
  }

  mHelpURLS = lst;
}
KURL::List
MailingList::helpURLS() const
{
  return mHelpURLS;
}

void
MailingList::setArchiveURLS( const KURL::List& lst )
{
  mFeatures |= Archive;
  if ( lst.empty() ) {
    mFeatures ^= Archive;
  }

  mArchiveURLS = lst;
}
KURL::List
MailingList::archiveURLS() const
{
  return mArchiveURLS;
}

void
MailingList::setId( const QString& str )
{
  mFeatures |= Id;
  if ( str.isEmpty() ) {
    mFeatures ^= Id;
  }

  mId = str;
}
QString
MailingList::id() const
{
  return mId;
}

void
MailingList::writeConfig( KConfig* config ) const
{
  config->writeEntry( "MailingListFeatures", mFeatures );
  config->writeEntry( "MailingListHandler", mHandler );
  config->writeEntry( "MailingListId",  mId );
  config->writeEntry( "MailingListPostingAddress", mPostURLS.toStringList() );
  config->writeEntry( "MailingListSubscribeAddress", mSubscribeURLS.toStringList() );
  config->writeEntry( "MailingListUnsubscribeAddress", mUnsubscribeURLS.toStringList() );
  config->writeEntry( "MailingListArchiveAddress", mArchiveURLS.toStringList() );
  config->writeEntry( "MailingListHelpAddress", mHelpURLS.toStringList() );
}

void
MailingList::readConfig( KConfig* config )
{
  mFeatures =  config->readNumEntry( "MailingListFeatures", 0 );
  mHandler = static_cast<MailingList::Handler>(
    config->readNumEntry( "MailingListHandler", MailingList::KMail ) );

  mId = config->readEntry("MailingListId");
  mPostURLS        = config->readListEntry( "MailingListPostingAddress" );
  mSubscribeURLS   = config->readListEntry( "MailingListSubscribeAddress" );
  mUnsubscribeURLS = config->readListEntry( "MailingListUnsubscribeAddress" );
  mArchiveURLS     = config->readListEntry( "MailingListArchiveAddress" );
  mHelpURLS        = config->readListEntry( "MailingListHelpAddress" );
}
