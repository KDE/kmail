#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kapplication.h"
#include "kstatusbar.h"
#include "kmkernel.h"
#include "kmsender.h"
#include "kmreaderwin.h"
#include "kmbroadcaststatus.h"
#include "klocale.h"
#include <kedittoolbar.h>
#include <kstdaction.h>
#include <qlayout.h>

#include "kmmainwin.moc"

KMMainWin::KMMainWin(QWidget *)
	: KMTopLevelWidget("kmail-mainwindow")
{
  mKMMainWidget = new KMMainWidget( this, "KMMainWidget", actionCollection() );
  mKMMainWidget->resize( 450, 600 );
  setCentralWidget(mKMMainWidget);
  setupStatusBar();

  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()),
				actionCollection(), "kmail_configure_toolbars" );
  mToolbarAction = KStdAction::showToolbar(this,
					   SLOT(slotToggleToolBar()),
					   actionCollection());
  mStatusbarAction = KStdAction::showStatusbar(this,
					       SLOT(slotToggleStatusBar()),
					       actionCollection());
  KStdAction::quit( this, SLOT(slotQuit()), actionCollection());
  createGUI( "kmmainwin.rc", false );
  mToolbarAction->setChecked(!toolBar()->isHidden());
  mStatusbarAction->setChecked(!statusBar()->isHidden());

  connect( guiFactory()->container("folder", this),
	   SIGNAL( aboutToShow() ), mKMMainWidget,
	   SLOT( updateFolderMenu() ));

  connect( guiFactory()->container("message", this),
	   SIGNAL( aboutToShow() ), mKMMainWidget,
	   SLOT( updateMessageMenu() ));

  // contains "Create Filter" actions.
  connect( guiFactory()->container("tools", this),
	   SIGNAL( aboutToShow() ), mKMMainWidget,
	   SLOT( updateMessageMenu() ));

  // contains "View source" action.
  connect( guiFactory()->container("view", this),
	   SIGNAL( aboutToShow() ), mKMMainWidget,
	   SLOT( updateMessageMenu() ));

  conserveMemory();
  applyMainWindowSettings(KMKernel::config(), "Main Window");
  connect(kernel->msgSender(), SIGNAL(statusMsg(const QString&)),
	  this, SLOT(statusMsg(const QString&)));
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
  QString text = " " + aText + " ";
  int statusWidth = statusBar()->width() - littleProgress->width()
    - fontMetrics().maxWidth();

  while (!text.isEmpty() && fontMetrics().width( text ) >= statusWidth)
    text.truncate( text.length() - 1);

  // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
  // but this code would double the size of the satus bar if the user hovers
  // over an <foo@bar.com>-style email address :-(
//  text.replace(QRegExp("&"), "&amp;");
//  text.replace(QRegExp("<"), "&lt;");
//  text.replace(QRegExp(">"), "&gt;");

  statusBar()->changeItem(text, mMessageStatusId);
}

void KMMainWin::slotToggleToolBar()
{
  if(toolBar("mainToolBar")->isVisible())
    toolBar("mainToolBar")->hide();
  else
    toolBar("mainToolBar")->show();
}

void KMMainWin::slotToggleStatusBar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
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
  mToolbarAction->setChecked(!toolBar()->isHidden());
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
  mKMMainWidget->readConfig();
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

