/* Authors: Don Sanders <sanders@kde.org>

   Copyright (C) 2000 Don Sanders <sanders@kde.org>


   License GPL
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qdatetime.h>

#include <klocale.h>
#include <kglobal.h>

#include "kmbroadcaststatus.h"
#include "progressmanager.h"


//-----------------------------------------------------------------------------
KMBroadcastStatus* KMBroadcastStatus::instance_ = 0;

KMBroadcastStatus* KMBroadcastStatus::instance()
{
  if (!instance_)
    instance_ = new KMBroadcastStatus();
  return instance_;
}

KMBroadcastStatus::KMBroadcastStatus()
{
  reset();
}

void KMBroadcastStatus::setStatusMsg( const QString& message )
{
  emit statusMsg( message );
}

void KMBroadcastStatus::setStatusMsgWithTimestamp( const QString& message )
{
  KLocale* locale = KGlobal::locale();
  setStatusMsg( i18n( "%1 is a time, %2 is a status message", "[%1] %2" )
                .arg( locale->formatTime( QTime::currentTime(),
                                          true /* with seconds */ ) )
                .arg( message ) );
}

void KMBroadcastStatus::setStatusMsgTransmissionCompleted( int numMessages,
                                                           int numBytes,
                                                           int numBytesRead,
                                                           int numBytesToRead,
                                                           bool mLeaveOnServer,
                                                           KMail::ProgressItem* item )
{
  QString statusMsg;
  if( numMessages > 0 ) {
    if( numBytes != -1 ) {
      if( ( numBytesToRead != numBytes ) && mLeaveOnServer )
        statusMsg = i18n( "Transmission complete. %n new message in %1 KB "
                          "(%2 KB remaining on the server).",
                          "Transmission complete. %n new messages in %1 KB "
                          "(%2 KB remaining on the server).",
                          numMessages )
                    .arg( numBytesRead / 1024 )
                    .arg( numBytes / 1024 );
      else
        statusMsg = i18n( "Transmission complete. %n message in %1 KB.",
                         "Transmission complete. %n messages in %1 KB.",
                          numMessages )
                    .arg( numBytesRead / 1024 );
    }
    else
      statusMsg = i18n( "Transmission complete. %n new message.",
                        "Transmission complete. %n new messages.",
                        numMessages );
  }
  else
    statusMsg = i18n( "Transmission complete. No new messages." );

  setStatusMsgWithTimestamp( statusMsg );
  if ( item )
    item->setStatus( statusMsg );
}

void KMBroadcastStatus::setStatusMsgTransmissionCompleted( const QString& account,
                                                           int numMessages,
                                                           int numBytes,
                                                           int numBytesRead,
                                                           int numBytesToRead,
                                                           bool mLeaveOnServer,
                                                           KMail::ProgressItem* item )
{
  QString statusMsg;
  if( numMessages > 0 ) {
    if( numBytes != -1 ) {
      if( ( numBytesToRead != numBytes ) && mLeaveOnServer )
        statusMsg = i18n( "Transmission for account %3 complete. "
                          "%n new message in %1 KB "
                          "(%2 KB remaining on the server).",
                          "Transmission for account %3 complete. "
                          "%n new messages in %1 KB "
                          "(%2 KB remaining on the server).",
                          numMessages )
                    .arg( numBytesRead / 1024 )
                    .arg( numBytes / 1024 )
                    .arg( account );
      else
        statusMsg = i18n( "Transmission for account %2 complete. "
                          "%n message in %1 KB.",
                          "Transmission for account %2 complete. "
                          "%n messages in %1 KB.",
                          numMessages )
                    .arg( numBytesRead / 1024 )
                    .arg( account );
    }
    else
      statusMsg = i18n( "Transmission for account %1 complete. "
                        "%n new message.",
                        "Transmission for account %1 complete. "
                        "%n new messages.",
                        numMessages )
                  .arg( account );
  }
  else
    statusMsg = i18n( "Transmission for account %1 complete. No new messages.")
                .arg( account );

  setStatusMsgWithTimestamp( statusMsg );
  if ( item )
    item->setStatus( statusMsg );
}

void KMBroadcastStatus::setStatusProgressEnable( const QString &id,
  bool enable )
{
  bool wasEmpty = ids.isEmpty();
  if (enable) ids.insert(id, 0);
  else ids.remove(id);
  if (!wasEmpty && !ids.isEmpty())
    setStatusProgressPercent("", 0);
  else
    emit statusProgressEnable( !ids.isEmpty() );
}

void KMBroadcastStatus::setStatusProgressPercent( const QString &id,
  unsigned long percent )
{
  if (!id.isEmpty() && ids.contains(id)) ids.insert(id, percent);
  unsigned long sum = 0, count = 0;
  for (QMap<QString,unsigned long>::iterator it = ids.begin();
    it != ids.end(); it++)
  {
    sum += *it;
    count++;
  }
  emit statusProgressPercent( count ? (sum / count) : sum );
}

void KMBroadcastStatus::reset()
{
  abortRequested_ = false;
  if (ids.isEmpty())
    emit resetRequested();
}

bool KMBroadcastStatus::abortRequested()
{
  return abortRequested_;
}

void KMBroadcastStatus::setAbortRequested()
{
  abortRequested_ = true;
}

void KMBroadcastStatus::requestAbort()
{
  abortRequested_ = true;
}
#include "kmbroadcaststatus.moc"

