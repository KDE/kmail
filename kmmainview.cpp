// Main View

#include <qstring.h>
#include <qpixmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qtstream.h>
#include <kmsgbox.h>
#include <kpanner.h>

#include "util.h"
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

  currentFolder = new KMFolder();
}


//-----------------------------------------------------------------------------
void KMMainView::initSeparated()
{
  vertPanner=new KPanner(this, NULL,KPanner::O_VERTICAL,30);
  connect(vertPanner, SIGNAL(positionChanged()),
	  this, SLOT(pannerHasChanged()));

  folderTree = new KMFolderTree(vertPanner->child0());
  connect(folderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));

  headers = new KMHeaders(vertPanner->child1());
  connect(headers, SIGNAL(messageSelected(KMMessage*)),
	  this, SLOT(messageSelected(KMMessage *)));

  horzPanner = NULL;
  Integrated = FALSE;

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

  folderTree = new KMFolderTree(vertPanner->child0());
  connect(folderTree, SIGNAL(folderSelected(KMFolder*)),
	  this, SLOT(folderSelected(KMFolder*)));

  headers = new KMHeaders(vertPanner->child1());
  connect(headers, SIGNAL(messageSelected(KMMessage*)),
	  this, SLOT(messageSelected(KMMessage*)));

  messageView = new KMReaderView(horzPanner->child1());
  Integrated = TRUE;
}


//-----------------------------------------------------------------------------
void KMMainView::doAddFolder() 
{
#ifdef BROKEN
  KMFolderDialog *d=new KMFolderDialog(this);
  d->setCaption("Create Folder");
  if (d->exec()) 
  {
    QDir dir;
    folderTree->cdFolder(&dir);
    dir.mkdir(d->nameEdit->text());
    folderTree->getList();
  }
  delete d;
#endif
}


//-----------------------------------------------------------------------------
void KMMainView::doCheckMail() 
{
  acctMgr->checkMail();
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
#ifdef BROKEN
  KMFolderDialog *d;

  if ((folderTree->currentItem())<1) 
    return; // make sure something is selected and it's not "/"

  d = new KMFolderDialog(this, currentFolder);
  d->setCaption("Modify Folder");
  d->nameEdit->setText(folderTree->getCurrentItem()->getText());

  if (d->exec())
  {
    QDir dir;
    folderTree->cdFolder(&dir);
    dir.cdUp();
    dir.rename(folderTree->getCurrentItem()->getText(),d->nameEdit->text());
    folderTree->getList();
  }
  delete d;
#endif
}


//-----------------------------------------------------------------------------
void KMMainView::doRemoveFolder()
{
#ifdef BROKEN
  if ((folderTree->currentItem())<1) return;

  QString s;
  QDir dir;
  s.sprintf("Are you sure you want to remove the folder \"%s\"\n and all of its child folders?",
	    folderTree->getCurrentItem()->getText());
  if ((KMsgBox::yesNo(this,"Confirmation",s))==1) {
    headers->clear();
    if ( horzPanner) messageView->clearCanvas();
    folderTree->cdFolder(&dir);
    removeDirectory(dir.path());
    folderTree->getList();
  }
#endif
}


//-----------------------------------------------------------------------------
void KMMainView::folderSelected(KMFolder* aFolder)
{
  if (currentFolder) currentFolder->close();
  currentFolder = aFolder;
  if (currentFolder) currentFolder->open();

  headers->setFolder(currentFolder);

  if (horzPanner) messageView->clearCanvas();
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
