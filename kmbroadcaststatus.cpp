/* Authors: Don Sanders <sanders@kde.org>

   Copyright (C) 2000 Don Sanders <sanders@kde.org>

   License GPL
*/

#include "kmbroadcaststatus.moc"

#include "kmbroadcaststatus.h"
#include <kprogress.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <klocale.h>

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

void KMBroadcastStatus::setStatusProgressEnable( bool enable )
{
  emit statusProgressEnable( enable );
}

void KMBroadcastStatus::setStatusProgressPercent( unsigned long percent )
{
  emit statusProgressPercent( percent );
}

void KMBroadcastStatus::reset()
{
  abortRequested_ = false;
  emit resetRequested();
}

bool KMBroadcastStatus::abortRequested()
{
  return abortRequested_;
}

void KMBroadcastStatus::requestAbort()
{
  abortRequested_ = true;
}


//-----------------------------------------------------------------------------
KMLittleProgressDlg::KMLittleProgressDlg( QWidget* parent, bool button )
  : QFrame( parent )
{
  m_bShowButton = button;
  setFrameStyle( QFrame::Panel | QFrame::Sunken );

  QFontMetrics fm = fontMetrics();
  int w_offset = fm.width( "X" ) + 10;
  int w = fm.width( " 999.9 kB/s 00:00:01 " ) + 8;
  int h = fm.height() + 2;
  //  int h = fm.height() + 3;

  m_pButton = new QPushButton( "X", this );
  //  m_pButton->setGeometry( 0, 1, w_offset, h - 1);
  m_pButton->setGeometry( 0, 0, w_offset, h + 2 );
  QToolTip::add( m_pButton, i18n("Cancel job") );
  
  m_pProgressBar = new KProgress( 0, 100, 0, KProgress::Horizontal, this );
  m_pProgressBar->setFrameStyle( QFrame::Box | QFrame::Raised );
  m_pProgressBar->setLineWidth( 1 );
  m_pProgressBar->setBackgroundMode( QWidget::PaletteBackground );
  m_pProgressBar->setBarColor( Qt::blue );
  m_pProgressBar->setGeometry( w_offset, 1, w + w_offset, h - 1 );
  m_pProgressBar->installEventFilter( this );

  m_pLabel = new QLabel( "", this );
  m_pLabel->setFrameStyle( QFrame::Box | QFrame::Raised );
  m_pLabel->setGeometry( w_offset, 1, w + w_offset, h - 1 );
  m_pLabel->installEventFilter( this );

  mode = None;
  setMode();

  resize( w + w_offset, h );

  connect( m_pButton, SIGNAL( clicked() ),
	   KMBroadcastStatus::instance(), SLOT( requestAbort() ));
}

void KMLittleProgressDlg::slotEnable( bool enabled )
{
  if (enabled) {
    mode = Progress;
    debug( "enable progress" );
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
    m_pProgressBar->hide();
    m_pLabel->hide();
    break;

  case Label:
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    m_pProgressBar->hide();
    m_pLabel->show();
    break;

  case Progress:
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    m_pProgressBar->show();
    m_pLabel->hide();
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
