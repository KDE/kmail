// Main View

#include <qstring.h>
#include <qpixmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <kpanner.h>
#include <klocale.h>

#include "kmcomposewin.h"
#include "kmsettings.h"
#include "kmfolderdia.h"
#include "kmaccount.h"
#include "kmacctmgr.h"
#include "kbusyptr.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmainview.h"
#include "kmglobal.h"
#include "kmfoldertree.h"
#include "kmheaders.h"
#include "kmreaderwin.h"
#include "kmacctfolder.h"
#include "kmmainwin.h"

#include "kmmainview.moc"

//-----------------------------------------------------------------------------
KMMainView::KMMainView(QWidget *parent, const char *name):
  KMMainViewInherited(parent,name)
{
  QString o;
  KConfig *config;

  config = app->getConfig();
  config->setGroup("Settings");
  o = config->readEntry("Reader");

  if( !o.isEmpty() && o.find("separated",0,false) ==0)
    initSeparated();
  else
    initIntegrated();

  currentFolder = NULL;
}


//-----------------------------------------------------------------------------
void KMMainView::initSeparated()
{
  vertPanner=new KPanner(NULL, NULL,KPanner::O_VERTICAL,30);
  connect(vertPanner, SIGNAL(positionChanged()),
	  this, SLOT(pannerHasChanged()));

  folderTree = new KMFolderTree(NULL, "foldertree-seperated");
  connect(folderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));
  folderTree->setCaption(nls->translate("KMail Folders"));
  folderTree->resize(folderTree->size());
  folderTree->show();

  headers = new KMHeaders(NULL,"Header List");
  connect(headers, SIGNAL(messageSelected(KMMessage*)),
	  this, SLOT(messageSelected(KMMessage *)));
  headers->setCaption(nls->translate("KMail Message List"));
  headers->resize(headers->size());
  headers->show();

  horzPanner = NULL;
  Integrated = FALSE;

  resize(10,10);
}


//-----------------------------------------------------------------------------
void KMMainView::initIntegrated()
{
  horzPanner=new KPanner(this, NULL,KPanner::O_HORIZONTAL,40);
  connect(horzPanner, SIGNAL(positionChanged()),
	  this, SLOT(pannerHasChanged()));

  vertPanner=new KPanner(horzPanner->child0(), NULL,KPanner::O_VERTICAL,30);
  connect(vertPanner, SIGNAL(positionChanged()),
	  this, SLOT(pannerHasChanged()));

  folderTree = new KMFolderTree(vertPanner->child0(), "foldertree-integrated");
  connect(folderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));

  headers = new KMHeaders(vertPanner->child1(), "Header List");
  connect(headers, SIGNAL(messageSelected(KMMessage*)),
	  this, SLOT(messageSelected(KMMessage*)));

  messageView = new KMReaderView(horzPanner->child1());

  if(((KMMainWin*)parentWidget())->showInline == true)
    messageView->showInline = true;
  else
    messageView->showInline = false;

  Integrated = TRUE;
}


//-----------------------------------------------------------------------------
void KMMainView::doAddFolder() 
{
  KMFolderDialog *d;

  d = new KMFolderDialog(NULL, this);
  d->setCaption(nls->translate("New Folder"));
  if (d->exec()) folderTree->reload();
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainView::doCheckMail() 
{
  kbp->busy();
  acctMgr->checkMail();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainView::doCompose()
{
  KMComposeWin *d;
  d = new KMComposeWin;
  d->show();
}


//-----------------------------------------------------------------------------
void KMMainView::doModifyFolder()
{
  KMFolderDialog *d;

  if (!currentFolder) return;
  if (currentFolder->isSystemFolder())
  {
    warning(nls->translate("Cannot modify a\nsystem folder."));
    return;
  }

  d = new KMFolderDialog((KMAcctFolder*)currentFolder, this);
  d->setCaption(nls->translate("Modify Folder"));
  if (d->exec()) folderTree->reload();
  delete d;
}


//-----------------------------------------------------------------------------
void KMMainView::doEmptyFolder()
{
  kbp->busy();
  currentFolder->expunge();
  currentFolder->open();
  headers->setFolder(currentFolder);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainView::doRemoveFolder()
{
  QString str;
  QDir dir;

  if (!currentFolder) return;
  if (currentFolder->isSystemFolder())
  {
    warning(nls->translate("Cannot remove a\nsystem folder."));
    return;
  }

  str.sprintf(nls->translate("Are you sure you want to remove the folder \""
			     "%s\"\nand all of its child folders?"),
			     (const char*)currentFolder->label());
  if ((KMsgBox::yesNo(this,nls->translate("Confirmation"),str))==1)
  {
#ifdef BROKEN
    headers->clear();
    if ( horzPanner) messageView->clearCanvas();
    folderTree->cdFolder(&dir);
    removeDirectory(dir.path());
    folderTree->getList();
#else
    warning("Removing of folders is\nstill under construction");
#endif
  }
}


//-----------------------------------------------------------------------------
void KMMainView::folderSelected(KMFolder* aFolder)
{
  kbp->busy();

  if (currentFolder) currentFolder->close();
  currentFolder = (KMAcctFolder*)aFolder;
  if (currentFolder) currentFolder->open();

  headers->setFolder(currentFolder);

  if (horzPanner) messageView->clearCanvas();

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMainView::doDeleteMessage()
{
  headers->toggleDeleteMsg();
}


//-----------------------------------------------------------------------------
void KMMainView::doForwardMessage()
{
  headers->forwardMsg();
}


//-----------------------------------------------------------------------------
void KMMainView::doReplyMessage()
{
  headers->replyToMsg();
}

//-----------------------------------------------------------------------------
void KMMainView::doReplyAllToMessage()
{
  headers->replyAllToMsg();
}

void KMMainView::doNextMsg()
{
  if(headers->currentItem() < 0 )
    return;
  headers->nextMsg();
}

void KMMainView::doPreviousMsg()
{
  if(headers->currentItem() < 0 )
    return;
  headers->previousMsg();
}


void KMMainView::doPrintMessage()
{ 
  if(headers->currentItem() < 0)
      return;
  else
    messageView->printMail();
}

//-----------------------------------------------------------------------------
void KMMainView::messageSelected(KMMessage *m)
{
  KMReaderWin *w;

  if(horzPanner) messageView->parseMessage(m);
  else
  {
    w = new KMReaderWin(0,0,headers->currentItem()+1,currentFolder);
    w->show();
  }
}


//-----------------------------------------------------------------------------
void KMMainView::pannerHasChanged()
{
  resizeEvent(NULL);
}


//-----------------------------------------------------------------------------
void KMMainView::resizeEvent(QResizeEvent *e)
{
  QWidget::resizeEvent(e);

  if(horzPanner)
  {
    horzPanner->setGeometry(0, 0, width(), height());
    vertPanner->resize(horzPanner->child0()->width(),
		       horzPanner->child0()->height());
    folderTree->resize(vertPanner->child0()->width(),
		       vertPanner->child0()->height());
    headers->resize(vertPanner->child1()->width(),
		    vertPanner->child1()->height());
    messageView->resize(horzPanner->child1()->width(), 
			horzPanner->child1()->height());
  }
  else
  {
    vertPanner->resize(width(), height());
    folderTree->resize(vertPanner->child0()->width(),
		       vertPanner->child0()->height());
    headers->resize(vertPanner->child1()->width(),
		    vertPanner->child1()->height());
  }
}


void KMMainView::slotViewChange()
{
  if(messageView->showInline)
    messageView->showInline = false;
  else
    messageView->showInline = true;
  messageView->updateDisplay();  
}

bool KMMainView::isInline()
{
  if(messageView->showInline)
    return true;
  else
    return false;
}
