/* Authors: Don Sanders <sanders@kde.org>

   Copyright (C) 2000 Don Sanders <sanders@kde.org>
   Includes KMLittleProgressDlg which is based on KIOLittleProgressDlg
   by Matt Koss <koss@miesto.sk>

   License GPL
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmbroadcaststatus.h"
#include "kmbroadcaststatus.moc"

#include "ssllabel.h"
using KMail::SSLLabel;
#include "progressmanager.h"
using KMail::ProgressItem;
using KMail::ProgressManager;

#include <kprogress.h>
#include <kiconloader.h>
#include <kdebug.h>


#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <klocale.h>
#include <qlayout.h>
#include <qwidgetstack.h>
#include <qdatetime.h>
#include <kmkernel.h> // for the progress dialog
#include "kmmainwidget.h"

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

void KMBroadcastStatus::setUsingSSL( bool isUsing )
{
  emit signalUsingSSL( isUsing );
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

void KMBroadcastStatus::setAbortRequested()
{
  abortRequested_ = true;
}

void KMBroadcastStatus::requestAbort()
{
  abortRequested_ = true;
  emit signalAbortRequested();
}


//-----------------------------------------------------------------------------
KMLittleProgressDlg::KMLittleProgressDlg( KMMainWidget* mainWidget, QWidget* parent, bool button )
  : QFrame( parent ), m_mainWidget( mainWidget ), mCurrentItem( 0 )
{
  m_bShowButton = button;
  int w = fontMetrics().width( " 999.9 kB/s 00:00:01 " ) + 8;
  box = new QHBoxLayout( this, 0, 0 );

  m_pButton = new QPushButton( this );
  m_pButton->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
                                         QSizePolicy::Minimum ) );
  m_pButton->setPixmap( SmallIcon( "up" ) );
  box->addWidget( m_pButton  );
  stack = new QWidgetStack( this );
  stack->setMaximumHeight( fontMetrics().height() );
  box->addWidget( stack );

  m_sslLabel = new SSLLabel( this );
  box->addWidget( m_sslLabel );

  QToolTip::add( m_pButton, i18n("Open detailed progress dialog") );

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
           mainWidget, SLOT( slotToggleProgressDialog() ) );

  connect ( ProgressManager::instance(), SIGNAL( progressItemAdded( ProgressItem * ) ),
            this, SLOT( slotProgressItemAdded( ProgressItem * ) ) );
  connect ( ProgressManager::instance(), SIGNAL( progressItemCompleted( ProgressItem * ) ),
            this, SLOT( slotProgressItemCompleted( ProgressItem * ) ) );
}

void KMLittleProgressDlg::slotProgressItemAdded( ProgressItem *item )
{
  slotEnable(true);
  if ( item->parent() ) return; // we are only interested in top level items
  if ( mCurrentItem ) {
    disconnect ( mCurrentItem, SIGNAL( progressItemProgress( ProgressItem *, unsigned int ) ),
                 this, SLOT( slotProgressItemProgress( ProgressItem *, unsigned int ) ) );
  }
  mCurrentItem = item;
  connect ( mCurrentItem, SIGNAL( progressItemProgress( ProgressItem *, unsigned int ) ),
            this, SLOT( slotProgressItemProgress( ProgressItem *, unsigned int ) ) );
}

void KMLittleProgressDlg::slotProgressItemCompleted( ProgressItem *item )
{
  if ( mCurrentItem && mCurrentItem == item ) {
    slotClean();
    mCurrentItem = 0;
  }
}

void KMLittleProgressDlg::slotProgressItemProgress( ProgressItem *item, unsigned int value )
{
  Q_ASSERT( item == mCurrentItem); // the only one we should be connected to
  m_pProgressBar->setValue( value );
}

void KMLittleProgressDlg::slotEnable( bool enabled )
{
  if (enabled) {
    if (mode == Progress) // it's already enabled
      return;
    m_pButton->setDown( false );
    mode = Progress;
    kdDebug(5006) << "enable progress" << endl;
  }
  else {
    if (mode == None)
      return;
    mode = None;
  }
  setMode();
}

void KMLittleProgressDlg::slotSetSSL( bool ssl )
{
  m_sslLabel->setEncrypted( ssl );
}

void KMLittleProgressDlg::setMode() {
  switch ( mode ) {
  case None:
    if ( m_bShowButton ) {
      m_pButton->hide();
    }
    m_sslLabel->setState( SSLLabel::Done );
    // show the empty label in order to make the status bar look better
    stack->show();
    stack->raiseWidget( m_pLabel );
    break;

  case Clean:
    if ( m_bShowButton ) {
      m_pButton->hide();
    }
    m_sslLabel->setState( SSLLabel::Clean );
    // show the empty label in order to make the status bar look better
    stack->show();
    stack->raiseWidget( m_pLabel );
    break;

  case Label:
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    m_sslLabel->setState( m_sslLabel->lastState() );
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
      m_sslLabel->setState( m_sslLabel->lastState() );
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

  mode = Clean;
  setMode();
}

bool KMLittleProgressDlg::eventFilter( QObject *, QEvent *ev )
{
  if ( ev->type() == QEvent::MouseButtonPress ) {
    QMouseEvent *e = (QMouseEvent*)ev;

    if ( e->button() == LeftButton && mode != Clean ) {    // toggle view on left mouse button

      // Hide the progress bar when the detailed one is showing
      if ( mode == Label ) {
        mode = Progress;
      } else if ( mode == Progress ) {
        mode = Label;
      }
      setMode();

      // Consensus seems to be that we should show/hide the fancy dialog when the user
      // clicks anywhere in the small one.
      m_mainWidget->slotToggleProgressDialog();
      return true;
    }
  }
  return false;
}
