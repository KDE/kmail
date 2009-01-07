

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kstatusbar.h"
#include "messagesender.h"
#include "progressdialog.h"
#include "statusbarprogresswidget.h"
#include "accountwizard.h"
#include "broadcaststatus.h"
#include "accountmanager.h"

#include <kapplication.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kstandardaction.h>
#include <kdebug.h>
#include <ktip.h>
#include <kicon.h>

#include "kmmainwin.moc"

KMMainWin::KMMainWin(QWidget *)
    : KXmlGuiWindow( 0 ),
      mReallyClose( false )
{
  setObjectName( "kmail-mainwindow#" );
  // Set this to be the group leader for all subdialogs - this means
  // modal subdialogs will only affect this dialog, not the other windows
  setAttribute( Qt::WA_GroupLeader );

  KAction *action  = new KAction( KIcon("window-new"), i18n("New &Window"), this );
  actionCollection()->addAction( "new_mail_client", action );
  connect( action, SIGNAL( triggered(bool) ), SLOT( slotNewMailReader() ) );

  resize( 700, 500 ); // The default size

  mKMMainWidget = new KMMainWidget( this, this, actionCollection() );
  setCentralWidget( mKMMainWidget );
  setupStatusBar();
  if ( kmkernel->xmlGuiInstance().isValid() )
    setComponentData( kmkernel->xmlGuiInstance() );

  setStandardToolBarMenuEnabled( true );

  KStandardAction::configureToolbars( this, SLOT( slotEditToolbars() ),
                                      actionCollection() );

  KStandardAction::keyBindings( mKMMainWidget, SLOT( slotEditKeys() ),
                                actionCollection() );

  KStandardAction::quit( this, SLOT( slotQuit() ), actionCollection() );
#ifdef Q_WS_MACX
 // ### This quits the application prematurely, for example when the composer window
 // ### is closed while the main application is minimized to the systray
  connect( qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()));
#endif
  createGUI( "kmmainwin.rc" );
  // Don't use conserveMemory() because this renders dynamic plugging
  // of actions unusable!

  //must be after createGUI, otherwise e.g toolbar settings are not loaded
  applyMainWindowSettings( KMKernel::config()->group( "Main Window") );

  connect( KPIM::BroadcastStatus::instance(), SIGNAL( statusMsg( const QString& ) ),
           this, SLOT( displayStatusMsg(const QString&) ) );

  connect( mKMMainWidget, SIGNAL( captionChangeRequest(const QString&) ),
           SLOT( setCaption(const QString&) ) );

  // Enable mail checks again (see destructor)
  kmkernel->enableMailCheck();

  if ( kmkernel->firstStart() )
    AccountWizard::start( kmkernel, this );

  if ( kmkernel->firstInstance() )
    QTimer::singleShot( 200, this, SLOT( slotShowTipOnStart() ) );
}

KMMainWin::~KMMainWin()
{
  saveMainWindowSettings( KMKernel::config()->group( "Main Window") );
  KMKernel::config()->sync();

  if ( mReallyClose ) {
    // Check if this was the last KMMainWin
    uint not_withdrawn = 0;
    foreach ( KMainWindow* window, KMainWindow::memberList() ) {
      if ( !window->isHidden() && window->isTopLevel() &&
           window != this && ::qobject_cast<KMMainWin *>( window ) )
        not_withdrawn++;
    }

    if ( not_withdrawn == 0 ) {
      kDebug(5006) <<"Closing last KMMainWin: stopping mail check";
      // Running KIO jobs prevent kapp from exiting, so we need to kill them
      // if they are only about checking mail (not important stuff like moving messages)
      kmkernel->abortMailCheck();
      kmkernel->acctMgr()->cancelMailCheck();
    }
  }
}

void KMMainWin::displayStatusMsg(const QString& aText)
{
  if ( !statusBar() || !mLittleProgress) return;
  int statusWidth = statusBar()->width() - mLittleProgress->width()
                    - fontMetrics().maxWidth();

  QString text = fontMetrics().elidedText( ' ' + aText, Qt::ElideRight,
                                           statusWidth );

  // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
  // but this code would double the size of the satus bar if the user hovers
  // over an <foo@bar.com>-style email address :-(
//  text.replace("&", "&amp;");
//  text.replace("<", "&lt;");
//  text.replace(">", "&gt;");

  statusBar()->changeItem(text, mMessageStatusId);
}

//-----------------------------------------------------------------------------
void KMMainWin::slotNewMailReader()
{
  KMMainWin *d;

  d = new KMMainWin();
  d->show();
  d->resize( d->size() );
}


void KMMainWin::slotEditToolbars()
{
  saveMainWindowSettings(KMKernel::config()->group( "Main Window") );
  KEditToolBar dlg(actionCollection(), this);
  dlg.setResourceFile( "kmmainwin.rc" );

  connect( &dlg, SIGNAL(newToolbarConfig()), SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMMainWin::slotUpdateToolbars()
{
  // remove dynamically created actions before editing
  mKMMainWidget->clearFilterActions();
  mKMMainWidget->clearMessageTagActions();//OnurAdd

  createGUI("kmmainwin.rc");
  applyMainWindowSettings(KMKernel::config()->group( "Main Window") );

  // plug dynamically created actions again
  mKMMainWidget->initializeFilterActions();
  mKMMainWidget->initializeMessageTagActions();
}

void KMMainWin::setupStatusBar()
{
  mMessageStatusId = 1;

  /* Create a progress dialog and hide it. */
  mProgressDialog = new KPIM::ProgressDialog( statusBar(), this );
  mProgressDialog->hide();

  mLittleProgress = new StatusbarProgressWidget( mProgressDialog, statusBar() );
  mLittleProgress->show();

  statusBar()->insertItem( i18n("Starting..."), 1, 4 );
  QTimer::singleShot( 2000, KPIM::BroadcastStatus::instance(), SLOT( reset() ) );
  statusBar()->setItemAlignment( 1, Qt::AlignLeft | Qt::AlignVCenter );
  statusBar()->addPermanentWidget( mKMMainWidget->vacationScriptIndicator() );
  statusBar()->addPermanentWidget( mLittleProgress );
  mLittleProgress->show();
}

void KMMainWin::slotQuit()
{
  mReallyClose = true;
  close();
}

//-----------------------------------------------------------------------------
bool KMMainWin::queryClose()
{
  if ( kmkernel->shuttingDown() || kapp->sessionSaving() || mReallyClose )
    return true;
  return kmkernel->canQueryClose();
}

void KMMainWin::slotShowTipOnStart()
{
  KTipDialog::showTip( this );
}


