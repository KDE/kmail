// kmfoldertree.cpp

#include <stdlib.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <kapp.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qpixmap.h>
#include <klocale.h>
#include <kglobal.h>

#include "kmglobal.h"
#include "kmdragdata.h"
#include "kmfoldermgr.h"
#include "kmfolderdir.h"
#include "kmfolder.h"

#include "kmfoldertree.moc"

//-----------------------------------------------------------------------------
KMFolderTree::KMFolderTree(QWidget *parent,const char *name):
  KMFolderTreeInherited(parent, name)
{
  KConfig* conf = app->config();
  static QPixmap pixDir, pixNode, pixPlain, pixFld, pixIn, pixOut, pixTr,
		 pixSent;
  static bool pixmapsLoaded = FALSE;
  int width;

  initMetaObject();
  mUpdateTimer = NULL;

  setAcceptDrops(true);

  connect(this, SIGNAL(selectionChanged(QListViewItem*)),
	  this, SLOT(doClicked(QListViewItem*)));
  connect(folderMgr, SIGNAL(changed()),
	  this, SLOT(doFolderListChanged()));
  connect(this, SIGNAL(popupMenu(int,int)),
          this, SLOT(slotRMB(int,int)));
  connect(folderMgr, SIGNAL(unreadChanged(KMFolder*)),
	  this, SLOT(refresh(KMFolder*)));

  conf->setGroup("Geometry");
  width = conf->readNumEntry(name, 80);
  resize(width, size().height());

  // clearFlags();
  // setFlags (Tbl_smoothVScrolling | Tbl_autoVScrollBar);

  addColumn(i18n("Folders"));
  addColumn(i18n(""));

  // setAutoUpdate(TRUE);
  reload();
}


//-----------------------------------------------------------------------------
QPixmap KMFolderTree::folderTypePixmap(const QString aType)
{
  KIconLoader* loader = KGlobal::iconLoader();
  const char* fname = 0;

  if (aType == "dir") fname = "kmflddir.xpm";
  else if (aType == "In") fname = "kmfldin.xpm";
  else if (aType == "Out") fname = "kmfldout.xpm";
  else if (aType == "St") fname = "kmfldsent.xpm";
  else if (aType == "Tr") fname = "kmtrash.xpm";
  else fname = "kmfolder.xpm";

  return loader->loadIcon(fname);
}


//-----------------------------------------------------------------------------
void KMFolderTree::updateUnreadAll()
{
  KMFolderDir* fdir;
  KMFolder* folder;
  debug( "KMFolderTree::updateUnreadAll" );
  //  bool upd = autoUpdate();
  //  setAutoUpdate(FALSE);

  fdir = &folderMgr->dir();
  for (folder = (KMFolder*)fdir->first();
    folder != NULL;
    folder = (KMFolder*)fdir->next())
  {
    folder->open();
    folder->countUnread();
    folder->close();
  }
  //  setAutoUpdate(upd);
}

//-----------------------------------------------------------------------------
KMFolderTree::~KMFolderTree()
{
  KConfig* conf = app->config();

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());

  disconnect(folderMgr, SIGNAL(changed()),
	     this, SLOT(doFolderListChanged()));
  disconnect(folderMgr, SIGNAL(unreadChanged(KMFolder*)),
	     this, SLOT(refresh(KMFolder*)));

  delete mUpdateTimer;
}


//-----------------------------------------------------------------------------
void KMFolderTree::setCurrentFolderNode(const KMFolderNode* aNode)
{
  KMFolderTreeItem* it = findItem(aNode);
  if (it) setCurrentItem(it);
}


//-----------------------------------------------------------------------------
KMFolderNode* KMFolderTree::currentFolderNode() const
{
  KMFolderTreeItem* cur = (KMFolderTreeItem*)currentItem();
  if (!cur) return NULL;
  return cur->node();
}


//-----------------------------------------------------------------------------
void KMFolderTree::reload(void)
{
  KMFolderTreeItem* rootItem;
  //  bool upd = autoUpdate();
  //  setAutoUpdate(FALSE);

  clear();
  rootItem = appendFolderNode(&folderMgr->dir());
  if (rootItem) rootItem->setOpen(true);

  //  setAutoUpdate(upd);
  //  if (upd) repaint();
}


//-----------------------------------------------------------------------------
KMFolderTreeItem* KMFolderTree::appendFolderNode(KMFolderNode* aNode,
						 KMFolderTreeItem* aParent)
{
  KMFolderTreeItem* item;
  KMFolderNode* node;
  KMFolderDir* fdir;
  QString type, label, str;
  KMFolder* fld = NULL;
  QPixmap pix;
  long numUnread = -1;

  label = aNode->label();
  if (label.isEmpty()) label = i18n("Local Folders");

  type = aNode->type();
  if (!aNode->isDir())
  {
    fld = (KMFolder*)aNode;
    fld->open();
    numUnread = fld->countUnread();
    fld->close();
  }

  if (aParent) item = new KMFolderTreeItem(aParent, label);
  else item = new KMFolderTreeItem(this, label);
  item->setNode(aNode);

  pix = folderTypePixmap(type);
  if (!pix.isNull()) item->setPixmap(0, pix);

  if (aNode->isDir())
  {
    fdir = (KMFolderDir*)aNode;
    for (node = fdir->first(); node; node = fdir->next())
      appendFolderNode(node, item);
  }
  else
  {
    str.setNum(numUnread);
    item->setText(1, str);
  }
  return item;
}


//-----------------------------------------------------------------------------
void KMFolderTree::refresh(KMFolder*)
{
  if (!mUpdateTimer)
  {
    mUpdateTimer = new QTimer(this);
    connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(delayedUpdate()));
  }
  mUpdateTimer->changeInterval(200);
}


//-----------------------------------------------------------------------------
KMFolderTreeItem* KMFolderTree::findItemRecursive(const KMFolderNode* aNode,
						  KMFolderTreeItem* aItem) const 
{
  KMFolderTreeItem* item;
  KMFolderTreeItem* child;
  KMFolderTreeItem* found;

  for (item = (KMFolderTreeItem*)aItem; item;
       item = (KMFolderTreeItem*)item->nextSibling())
  {
    if (item->node() == aNode) return item;
    child = (KMFolderTreeItem*)item->firstChild();
    if (child)
    {
      found = findItemRecursive(aNode, child);
      if (found) return found;
    }
  }
  return NULL;
}


//-----------------------------------------------------------------------------
KMFolderTreeItem* KMFolderTree::findItem(const KMFolderNode* aNode) const 
{
  return findItemRecursive(aNode, (KMFolderTreeItem*)firstChild());
}


//-----------------------------------------------------------------------------
void KMFolderTree::delayedUpdate()
{
#ifdef BROKEN
  int i;
  KMFolder* folder;
  QString str;
  bool upd = autoUpdate();
  bool repaintRequired = false;

  setAutoUpdate(FALSE);

  for (i=0, folder = (KMFolder*)mList.first();
    folder != NULL;
    folder = (KMFolder*)mList.next(),i++)
  {
    str = QString("{") + folder->type() + "} " + folder->label();
    if (text(i) != str) {
       repaintRequired = true;
       changeItem(str, i);
    if (folder->countUnread()>0)
       changeItemColor(darkRed, i);
    else
       changeItemColor(kapp->palette().normal().foreground(), i);
    }
  }
  setAutoUpdate(upd);
  if (upd && repaintRequired) repaint();

  mUpdateTimer->stop();
#endif
}

//-----------------------------------------------------------------------------
void KMFolderTree::doFolderListChanged()
{
  //  QListViewItem* it = currentItem();
  debug("doFolderListChanged()");
  reload();
  //  if (idx >= 0 && idx < count()) setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KMFolderTree::dropEvent(QDropEvent * event)
{
#if 0
  KMFolder *toFld, *fromFld;
  KMDragData* dd;
  KMMessage* msg;
  QPoint pos;
  int i;

  if (aDropZone!=mDropZone) return;
  if (aDropZone->getDataType() != DndRawData) return; //sven

  dd = (KMDragData*)aDropZone->getData().ascii();
  if (!dd || sizeof(*dd)!=sizeof(KMDragData)) return;
  fromFld = dd->folder();

  pos.setY(aDropZone->getMouseY());
  pos = mapFromGlobal(pos);

  toFld = (KMFolder*)mList.at(findItem(pos.y()));
  if (!fromFld || !toFld || toFld->isDir()) return;

  fromFld->open();
  toFld->open();

  for (i=dd->to(); i>=dd->from(); i--)
  {
    msg = fromFld->take(i);
    if (msg) toFld->addMsg(msg);
  }

  fromFld->close();
  toFld->close();
#endif
}


//-----------------------------------------------------------------------------
void KMFolderTree::inSort(KMFolder* aFolder)
{
#ifdef BROKEN
  KMFolder* cur;
  QString str;
  int i, cmp;

  for (i=0,cur=(KMFolder*)mList.first(); cur; cur=(KMFolder*)mList.next(),i++)
  {
    cmp = strcmp(aFolder->type(), cur->type());
    if (!cmp) cmp = stricmp(aFolder->label(), cur->label());
    if (cmp < 0) break;
  }

  str = QString("{") + aFolder->type() + "} " + aFolder->label();
  insertItem(str, i);
  mList.insert(i, aFolder);

  if (aFolder->countUnread()>0)
    changeItemColor(darkRed, i);
  else
    changeItemColor(kapp->palette().normal().foreground(), i);
#endif
}


//-----------------------------------------------------------------------------
void KMFolderTree::doClicked(QListViewItem* aItem)
{
  KMFolderTreeItem* it = (KMFolderTreeItem*)aItem;
  KMFolderNode* node;
  KMFolder* fld = NULL;

  if (!it) return;
  if (!(node = it->node())) return;

  if (!node->isDir()) fld = (KMFolder*)node;

  debug("doClicked: %s\n", fld ? (const char*)fld->label() : "(none)");
  emit folderSelected(fld);
}


//-----------------------------------------------------------------------------
void KMFolderTree::resizeEvent(QResizeEvent* e)
{
  KConfig* conf = app->config();

  conf->setGroup("Geometry");
  conf->writeEntry(name(), size().width());

  KMFolderTreeInherited::resizeEvent(e);
}


//-----------------------------------------------------------------------------
void KMFolderTree::slotRMB(int index, int)
{
#ifdef BROKEN
  doFolderSelected(index, 0);
  setCurrentItem(index);

  if (!topLevelWidget()) return; // safe bet

  QPopupMenu *folderMenu = new QPopupMenu;

  folderMenu->insertItem(i18n("&Create..."), topLevelWidget(),
              SLOT(slotAddFolder()));
  folderMenu->insertItem(i18n("&Modify..."), topLevelWidget(),
              SLOT(slotModifyFolder()));
  folderMenu->insertItem(i18n("C&ompact"), topLevelWidget(),
              SLOT(slotCompactFolder()));
  folderMenu->insertSeparator();
  folderMenu->insertItem(i18n("&Empty"), topLevelWidget(),
              SLOT(slotEmptyFolder()));
  folderMenu->insertItem(i18n("&Remove"), topLevelWidget(),
              SLOT(slotRemoveFolder()));
  folderMenu->exec (QCursor::pos(), 0);
  delete folderMenu;
#endif
}


//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem(KMFolderTreeItem* i, const QString& str):
  QListViewItem(i, str)
{
}


//-----------------------------------------------------------------------------
KMFolderTreeItem::KMFolderTreeItem(KMFolderTree* t, const QString& str):
  QListViewItem(t, str)
{
}

