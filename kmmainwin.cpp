#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kstatusbar.h"
#include "kmkernel.h"
#include "kmsender.h"
#include "kmbroadcaststatus.h"
#include "kmglobal.h"
#include "kapplication.h"
#include <klocale.h>
#include <kedittoolbar.h>
#include <kconfig.h>
#include <kmessagebox.h>

#include "kmmainwin.moc"

KMMainWin::KMMainWin(QWidget *)
	: KMTopLevelWidget("kmail-mainwindow")
{
  mKMMainWidget = new KMMainWidget( this, "KMMainWidget", actionCollection() );
  mKMMainWidget->resize( 450, 600 );
  setCentralWidget(mKMMainWidget);
  setupStatusBar();

#if KDE_IS_VERSION( 3, 1, 90 )
  createStandardStatusBarAction();
  setStandardToolBarMenuEnabled(true);
#endif

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()),
				actionCollection(), "kmail_configure_toolbars" );

  KStdAction::keyBindings(mKMMainWidget, SLOT(slotEditKeys()),
                          actionCollection());

#if !KDE_IS_VERSION( 3, 1, 90 )
  mToolbarAction = KStdAction::showToolbar(this,
					   SLOT(slotToggleToolBar()),
					   actionCollection());
  mStatusbarAction = KStdAction::showStatusbar(this,
					       SLOT(slotToggleStatusBar()),
					       actionCollection());
#endif

  KStdAction::quit( this, SLOT(slotQuit()), actionCollection());
  createGUI( "kmmainwin.rc", false );
#if !KDE_IS_VERSION( 3, 1, 90 )
  mToolbarAction->setChecked(!toolBar()->isHidden());
  mStatusbarAction->setChecked(!statusBar()->isHidden());
#endif

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
}

KMMainWin::~KMMainWin()
{
  saveMainWindowSettings(KMKernel::config(), "Main Window");
  KMKernel::config()->sync();
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

void KMMainWin::slotToggleToolBar()
{
#if !KDE_IS_VERSION( 3, 1, 90 )
  if(toolBar("mainToolBar")->isVisible())
    toolBar("mainToolBar")->hide();
  else
    toolBar("mainToolBar")->show();
#endif
}

void KMMainWin::slotToggleStatusBar()
{
#if !KDE_IS_VERSION( 3, 1, 90 )
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
#endif
}

void KMMainWin::slotEditToolbars()
{
  saveMainWindowSettings(KMKernel::config(), "MainWindow");
  KEditToolbar dlg(actionCollection(), "kmmainwin.rc");

  connect( &dlg, SIGNAL(newToolbarConfig()),
	   SLOT(slotUpdateToolbars()) );

  dlg.exec();
}

void KMMainWin::slotUpdateToolbars()
{
  createGUI("kmmainwin.rc");
  applyMainWindowSettings(KMKernel::config(), "MainWindow");
#if !KDE_IS_VERSION( 3, 1, 90 )
  mToolbarAction->setChecked(!toolBar()->isHidden());
#endif
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
  if (kmkernel->shuttingDown() || kapp->sessionSaving())
    return true;

  int      ret = 0;
  QString  str = i18n("Expire old messages from all folders? "
		      "Expired messages are permanently deleted.");
  KConfig *config = KMKernel::config();

  // Make sure this is the last window.
  KMainWindow   *kmWin = 0;
  int           num = 0;

  kmkernel->setCanExpire(false);
  for (kmWin = KMainWindow::memberList->first(); kmWin;
       kmWin = KMainWindow::memberList->next()) {
    if (kmWin->isA("KMMainWin")) {
      num++;
    }
  }
  // If this isn't the last open window, don't do anything.
  if (num > 1) {
    return true;
  }

  KConfigGroupSaver saver(config, "General");
  if (config->readNumEntry("when-to-expire", 0) != expireAtExit) {
    return true;
  }

  if (config->readBoolEntry("warn-before-expire", true)) {
    ret = KMessageBox::warningYesNo(KMainWindow::memberList->first(),
			 str, i18n("Expire Old Messages?"), i18n("Expire"), i18n("Don't Expire"));
    if (ret == KMessageBox::Continue) {
      kmkernel->setCanExpire(true);
    }
  }

  return true;
}
