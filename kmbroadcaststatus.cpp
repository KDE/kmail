/* Authors: Don Sanders <sanders@kde.org>

   Copyright (C) 2000 Don Sanders <sanders@kde.org>
   Includes KMLittleProgressDlg which is based on KIOLittleProgressDlg 
   by Matt Koss <koss@miesto.sk>

   License GPL
*/

#include "kmbroadcaststatus.moc"

#include <kprogress.h>
#include <kdebug.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <klocale.h>
#include <qlayout.h>
#include <qwidgetstack.h>
#include <qdatetime.h>

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
                                                           bool mLeaveOnServer )
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
}

void KMBroadcastStatus::setStatusMsgTransmissionCompleted( const QString& account,
                                                           int numMessages,
                                                           int numBytes,
                                                           int numBytesRead,
                                                           int numBytesToRead,
                                                           bool mLeaveOnServer )
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

void KMBroadcastStatus::requestAbort()
{
  abortRequested_ = true;
  emit signalAbortRequested();
}


//-----------------------------------------------------------------------------
KMLittleProgressDlg::KMLittleProgressDlg( QWidget* parent, bool button )
  : QFrame( parent )
{
  m_bShowButton = button;
  int w = fontMetrics().width( " 999.9 kB/s 00:00:01 " ) + 8;
  box = new QHBoxLayout( this, 0, 0 );

  m_pButton = new QPushButton( "X", this );
  m_pButton->setMinimumWidth(fontMetrics().width("XXXX"));
  box->addWidget( m_pButton  );
  stack = new QWidgetStack( this );
  stack->setMaximumHeight( fontMetrics().height() );
  box->addWidget( stack );

  QToolTip::add( m_pButton, i18n("Cancel job") );
  
  m_pProgressBar = new KProgress( this );
  m_pProgressBar->setLineWidth( 1 );
  m_pProgressBar->setFrameStyle( QFrame::Box );
  m_pProgressBar->installEventFilter( this );
  m_pProgressBar->setMinimumWidth( w );
  stack->addWidget( m_pProgressBar, 1 );

  m_pLabel = new QLabel( "", this );
  m_pLabel->setAlignment( AlignHCenter | AlignVCenter );
  m_pLabel->installEventFilter( this );
  m_pLabel->setMinimumWidth( w );
  stack->addWidget( m_pLabel, 2 );
  m_pButton->setMaximumHeight( fontMetrics().height() );
  setMinimumWidth( minimumSizeHint().width() );

  mode = None;
  setMode();

  connect( m_pButton, SIGNAL( clicked() ),
	   KMBroadcastStatus::instance(), SLOT( requestAbort() ));
}

void KMLittleProgressDlg::slotEnable( bool enabled )
{
  if (enabled) {
    m_pButton->setDown( false );
    mode = Progress;
    kdDebug(5006) << "enable progress" << endl;
  }
  else {
    mode = None;
  }
  setMode();
}

void KMLittleProgressDlg::setMode() {
  switch ( mode ) {
  case None:
    if ( m_bShowButton ) {
      m_pButton->hide();
    }
    // show the empty label in order to make the status bar look better
    stack->show();
    stack->raiseWidget( m_pLabel );
    break;

  case Label:
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    stack->show();
    stack->raiseWidget( m_pLabel );
    break;

  case Progress:
    if (stack->isVisible()) {
      stack->show();
      stack->raiseWidget( m_pProgressBar );
      if ( m_bShowButton ) {
        m_pButton->show();
      }
    }
    break;
  }
}


void KMLittleProgressDlg::slotJustPercent( unsigned long _percent )
{
  m_pProgressBar->setValue( _percent );
}

void KMLittleProgressDlg::slotClean()
{
  m_pProgressBar->setValue( 0 );
  m_pLabel->clear();

  mode = None;
  setMode();
}

bool KMLittleProgressDlg::eventFilter( QObject *, QEvent *ev ) 
{
  if ( ev->type() == QEvent::MouseButtonPress ) {
    QMouseEvent *e = (QMouseEvent*)ev;

    if ( e->button() == LeftButton ) {    // toggle view on left mouse button
      if ( mode == Label ) {
	mode = Progress;
      } else if ( mode == Progress ) {
	mode = Label;
      }
      setMode();
      return true;

    }
  }

  return false;
}
