#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kstatusbar.h"
#include "kmkernel.h"
#include "kmsender.h"
#include "kmbroadcaststatus.h"
#include "kmglobal.h"
#include "kmacctmgr.h"
#include <kapplication.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "kmmainwin.moc"

KMMainWin::KMMainWin(QWidget *)
    : KMainWindow( 0, "kmail-mainwindow#" )
{
  kapp->ref();
  mKMMainWidget = new KMMainWidget( this, "KMMainWidget", actionCollection() );
  mKMMainWidget->resize( 450, 600 );
  setCentralWidget(mKMMainWidget);
  setupStatusBar();
  if (kmkernel->xmlGuiInstance())
    setInstance( kmkernel->xmlGuiInstance() );

  setStandardToolBarMenuEnabled(true);

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()),
				actionCollection());

  KStdAction::keyBindings(mKMMainWidget, SLOT(slotEditKeys()),
                          actionCollection());

  KStdAction::quit( this, SLOT(slotQuit()), actionCollection());
  createGUI( "kmmainwin.rc", false );

  conserveMemory();
  applyMainWindowSettings(KMKernel::config(), "Main Window");
  connect(kmkernel->msgSender(), SIGNAL(statusMsg(const QString&)),
	  this, SLOT(statusMsg(const QString&)));
  connect(kmkernel, SIGNAL(configChanged()),
    this, SLOT(slotConfigChanged()));
  connect(mKMMainWidget->messageView(), SIGNAL(statusMsg(const QString&)),
	  this, SLOT(htmlStatusMsg(const QString&)));
  connect(mKMMainWidget, SIGNAL(captionChangeRequest(const QString&)),
	  SLOT(setCaption(const QString&)) );
  connect(mKMMainWidget, SIGNAL(modifiedToolBarConfig()),
	   SLOT(slotUpdateToolbars()) );

  // Enable mail checks again (see destructor)
  KMBroadcastStatus::instance()->reset();
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
      KMBroadcastStatus::instance()->setAbortRequested();
      kmkernel->acctMgr()->cancelMailCheck();
    }
  }
}

void KMMainWin::statusMsg(const QString& aText)
{
  mLastStatusMsg = aText;
  displayStatusMsg(aText);
}

void KMMainWin::htmlStatusMsg(const QString& aText)
{
  if (aText.isEmpty()) displayStatusMsg(mLastStatusMsg);
  else displayStatusMsg(aText);
}

void KMMainWin::displayStatusMsg(const QString& aText)
{
  if ( !statusBar() || !littleProgress) return;
  QString text = " " + aText + " ";
  int statusWidth = statusBar()->width() - littleProgress->width()
    - fontMetrics().maxWidth();

  while (!text.isEmpty() && fontMetrics().width( text ) >= statusWidth)
    text.truncate( text.length() - 1);

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
  createGUI("kmmainwin.rc");
  applyMainWindowSettings(KMKernel::config(), "Main Window");
}

void KMMainWin::setupStatusBar()
{
  mMessageStatusId = 1;
  littleProgress = mainKMWidget()->progressDialog();

  statusBar()->addWidget( littleProgress, 0 , true );
  statusBar()->insertItem(i18n(" Initializing..."), 1, 1 );
  statusBar()->setItemAlignment( 1, AlignLeft | AlignVCenter );
  littleProgress->show();
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
    close();
}

void KMMainWin::slotConfigChanged()
{
  readConfig();
}

//-----------------------------------------------------------------------------
bool KMMainWin::queryClose() {
#if 0
  if (kmkernel->shuttingDown() || kapp->sessionSaving())
    return true;
  // ask questions here, if any
#endif
  return true;
}
