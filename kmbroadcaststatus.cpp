/* Authors: Don Sanders <sanders@kde.org>

   Copyright (C) 2000 Don Sanders <sanders@kde.org>
   Includes KMLittleProgressDlg which is based on KIOLittleProgressDlg 
   by Matt Koss <koss@miesto.sk>

   License GPL
*/

#include "kmbroadcaststatus.moc"

#include "kmbroadcaststatus.h"
#include <kprogress.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <klocale.h>
#include <qlayout.h>
#include <qwidgetstack.h>

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
  int w = fontMetrics().width( " 999.9 kB/s 00:00:01 " ) + 8;
  box = new QHBoxLayout( this, 0, 0 );

  m_pButton = new QPushButton( "X", this );
  box->addWidget( m_pButton  );
  stack = new QWidgetStack( this );
  box->addWidget( stack );

  QToolTip::add( m_pButton, i18n("Cancel job") );
  
  m_pProgressBar = new KProgress( 0, 100, 0, KProgress::Horizontal, this );
  m_pProgressBar->setFrameStyle( QFrame::Box | QFrame::Raised );
  m_pProgressBar->setLineWidth( 1 );
  m_pProgressBar->setBackgroundMode( QWidget::PaletteBackground );
  m_pProgressBar->setBarColor( Qt::blue );
  m_pProgressBar->installEventFilter( this );
  m_pProgressBar->setMinimumWidth( w );
  stack->addWidget( m_pProgressBar, 1 );

  m_pLabel = new QLabel( "", this );
  m_pLabel->setAlignment( AlignHCenter | AlignVCenter );
  m_pLabel->installEventFilter( this );
  m_pLabel->setMinimumWidth( w );
  stack->addWidget( m_pLabel, 2 );
  m_pButton->setMaximumHeight( minimumSizeHint().height() - 8 ); // yes this is an evil cludge
  setMinimumWidth( minimumSizeHint().width() );

  mode = None;
  setMode();

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
    stack->hide();
    break;

  case Label:
    if ( m_bShowButton ) {
      m_pButton->show();
    }
    stack->show();
    stack->raiseWidget( m_pLabel );
    break;

  case Progress:
    stack->show();
    stack->raiseWidget( m_pProgressBar );
    if ( m_bShowButton ) {
      m_pButton->show();
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
