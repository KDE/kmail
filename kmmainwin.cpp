
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kstatusbar.h"
#include "kmkernel.h"
#include "kmsender.h"
#include "progressdialog.h"
#include "statusbarprogresswidget.h"
#include "broadcaststatus.h"
#include "kmglobal.h"
#include "kmacctmgr.h"
#include <kapplication.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kdebug.h>
#include <ktip.h>

#include "kmmainwin.moc"

KMMainWin::KMMainWin(QWidget *)
    : KMainWindow( 0, "kmail-mainwindow#" ),
      mReallyClose( false )
{
  // Set this to be the group leader for all subdialogs - this means
  // modal subdialogs will only affect this dialog, not the other windows
  setWFlags( getWFlags() | WGroupLeader );

  kapp->ref();
  mKMMainWidget = new KMMainWidget( this, "KMMainWidget", this, actionCollection() );
  mKMMainWidget->resize( 450, 600 );
  setCentralWidget(mKMMainWidget);
  setupStatusBar();
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );

  if ( kmkernel->firstInstance() )
    QTimer::singleShot( 200, this, SLOT(slotShowTipOnStart()) );

  setStandardToolBarMenuEnabled(true);

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()),
				actionCollection());

  KStdAction::keyBindings(mKMMainWidget, SLOT(slotEditKeys()),
                          actionCollection());

  KStdAction::quit( this, SLOT(slotQuit()), actionCollection());
  createGUI( "kmmainwin.rc", false );
  // Don't use conserveMemory() because this renders dynamic plugging
  // of actions unusable!

  applyMainWindowSettings(KMKernel::config(), "Main Window");

  connect( KPIM::BroadcastStatus::instance(), SIGNAL( statusMsg( const QString& ) ),
           this, SLOT( displayStatusMsg(const QString&) ) );

  connect(kmkernel, SIGNAL(configChanged()),
    this, SLOT(slotConfigChanged()));

  connect(mKMMainWidget, SIGNAL(captionChangeRequest(const QString&)),
	  SLOT(setCaption(const QString&)) );

  // Enable mail checks again (see destructor)
  kmkernel->enableMailCheck();
}

KMMainWin::~KMMainWin()
{
  saveMainWindowSettings(KMKernel::config(), "Main Window");
  KMKernel::config()->sync();
  kapp->deref();

  if ( !kmkernel->haveSystemTrayApplet() ) {
    // Check if this was the last KMMainWin
    int not_withdrawn = 0;
    QPtrListIterator<KMainWindow> it(*KMainWindow::memberList);
    for (it.toFirst(); it.current(); ++it){
      if ( !it.current()->isHidden() &&
           it.current()->isTopLevel() &&
           it.current() != this &&
           ::qt_cast<KMMainWin *>( it.current() )
        )
        not_withdrawn++;
    }

    if ( not_withdrawn == 0 ) {
      kdDebug(5006) << "Closing last KMMainWin: stopping mail check" << endl;
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
  QString text = KStringHandler::rPixelSqueeze( " " + aText, fontMetrics(),
                                                statusWidth );

  // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
  // but this code would double the size of the satus bar if the user hovers
  // over an <foo@bar.com>-style email address :-(
//  text.replace("&", "&amp;");
//  text.replace("<", "&lt;");
//  text.replace(">", "&gt;");

  statusBar()->changeItem(text, mMessageStatusId);
}

void KMMainWin::slotEditToolbars()
{
  saveMainWindowSettings(KMKernel::config(), "Main Window");
  KEditToolbar dlg(actionCollection(), "kmmainwin.rc");

  connect( &dlg, SIGNAL(newToolbarConfig()),
	   SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMMainWin::slotUpdateToolbars()
{
  // remove dynamically created actions before editing
  mKMMainWidget->clearFilterActions();

  createGUI("kmmainwin.rc", false);
  applyMainWindowSettings(KMKernel::config(), "Main Window");

  // plug dynamically created actions again
  mKMMainWidget->initializeFilterActions();
}

void KMMainWin::setupStatusBar()
{
  mMessageStatusId = 1;

  /* Create a progress dialog and hide it. */
  mProgressDialog = new KPIM::ProgressDialog( statusBar(), this );
  mProgressDialog->hide();

  mLittleProgress = new StatusbarProgressWidget( mProgressDialog, statusBar() );
  mLittleProgress->show();

  statusBar()->addWidget( mLittleProgress, 0 , true );
  statusBar()->insertItem(i18n(" Initializing..."), 1, 1 );
  statusBar()->setItemAlignment( 1, AlignLeft | AlignVCenter );
  mLittleProgress->show();
}

/** Read configuration options after widgets are created. */
void KMMainWin::readConfig(void)
{
}

/** Write configuration options. */
void KMMainWin::writeConfig(void)
{
  mKMMainWidget->writeConfig();
}

void KMMainWin::slotQuit()
{
  mReallyClose = true;
  close();
}

void KMMainWin::slotConfigChanged()
{
  readConfig();
}

//-----------------------------------------------------------------------------
bool KMMainWin::queryClose()
{
  if ( kapp->sessionSaving() )
    writeConfig();

  if ( kmkernel->shuttingDown() || kapp->sessionSaving() || mReallyClose )
    return true;
  return kmkernel->canQueryClose();
}

void KMMainWin::slotShowTipOnStart()
{
  KTipDialog::showTip( this );
}


