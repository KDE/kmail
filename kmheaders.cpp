// kmheaders.cpp

#include <qfiledlg.h>

#include "kmfolder.h"
#include "kmheaders.h"
#include "kmmessage.h"
#include "kbusyptr.h"
#include "kmdragdata.h"
#include "kmglobal.h"
#include "kmmainwin.h"
#include "kmcomposewin.h"
#include "kfileio.h"
#include "kmfiltermgr.h"

#include <drag.h>
#include <qstrlist.h>
#include <kapp.h>
#include <kiconloader.h>
#include <kapp.h>


//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(KMMainWin *aOwner, QWidget *parent, 
		     const char *name) :
  KMHeadersInherited(parent, name, 4)
{
  KIconLoader* loader = app->getIconLoader();
  static QPixmap pixNew, pixUns, pixDel, pixOld, pixRep, pixSent, pixQueued,
                 pixFwd;

  mOwner  = aOwner;
  mFolder = NULL;
  getMsgIndex = -1;
  mSortCol = KMMsgList::sfDate;
  mSortDescending = FALSE;

  setColumn(0, i18n("F"), 17, KMHeadersInherited::PixmapColumn);
  setColumn(1, i18n("Sender"), 200);
  setColumn(2, i18n("Subject"), 270);
  setColumn(3, i18n("Date"), 300);
  readConfig();

  pixNew   = loader->loadIcon("kmmsgnew.xpm");
  pixUns   = loader->loadIcon("kmmsgunseen.xpm");
  pixDel   = loader->loadIcon("kmmsgdel.xpm");
  pixOld   = loader->loadIcon("kmmsgold.xpm");
  pixRep   = loader->loadIcon("kmmsgreplied.xpm");
  pixQueued= loader->loadIcon("kmmsgqueued.xpm");
  pixSent  = loader->loadIcon("kmmsgsent.xpm");
  pixFwd   = loader->loadIcon("kmmsgforwarded.xpm");

  dict().insert(KMMsgBase::statusToStr(KMMsgStatusNew), &pixNew);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusUnread), &pixUns);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusDeleted), &pixDel);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusOld), &pixOld);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusReplied), &pixRep);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusForwarded), &pixFwd);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusQueued), &pixQueued);
  dict().insert(KMMsgBase::statusToStr(KMMsgStatusSent), &pixSent);

  connect(this,SIGNAL(selected(int,int)),
	  this,SLOT(selectMessage(int,int)));
  connect(this,SIGNAL(highlighted(int,int)),
	  this,SLOT(highlightMessage(int,int)));
  connect(this,SIGNAL(headerClicked(int)),
	  this,SLOT(headerClicked(int)));
}


//-----------------------------------------------------------------------------
KMHeaders::~KMHeaders ()
{
  if (mFolder)
  {
    writeFolderConfig();
    mFolder->close();
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::readFolderConfig (void)
{
  KConfig* config = app->getConfig();
  assert(mFolder!=NULL);

  config->setGroup("Folder-" + mFolder->name());
  setColumnWidth(1, config->readNumEntry("SenderWidth", 200));
  setColumnWidth(2, config->readNumEntry("SubjectWidth", 270));
  setColumnWidth(3, config->readNumEntry("DateWidth", 300));
  mSortCol = config->readNumEntry("SortColumn", (int)KMMsgList::sfDate);
  mSortDescending = (mSortCol < 0);
  mSortCol = abs(mSortCol);
  sort();
}


//-----------------------------------------------------------------------------
void KMHeaders::writeFolderConfig (void)
{
  KConfig* config = app->getConfig();
  assert(mFolder!=NULL);

  config->setGroup("Folder-" + mFolder->name());
  config->writeEntry("SenderWidth", columnWidth(1));
  config->writeEntry("SubjectWidth", columnWidth(2));
  config->writeEntry("DateWidth", columnWidth(3));
  config->writeEntry("SortColumn", (mSortDescending ? -mSortCol : mSortCol));
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (KMFolder *aFolder)
{
  if (mFolder) 
  {
    mFolder->close();
    writeFolderConfig();
    disconnect(mFolder, SIGNAL(msgHeaderChanged(int)),
	       this, SLOT(msgHeaderChanged(int)));
    disconnect(mFolder, SIGNAL(msgAdded(int)),
	       this, SLOT(msgAdded(int)));
    disconnect(mFolder, SIGNAL(msgRemoved(int)),
	       this, SLOT(msgRemoved(int)));
    disconnect(mFolder, SIGNAL(changed()),
	       this, SLOT(msgChanged()));
    disconnect(mFolder, SIGNAL(statusMsg(const char*)), 
	       mOwner, SLOT(statusMsg(const char*)));
  }

  mFolder = aFolder;

  if (mFolder)
  {
    connect(mFolder, SIGNAL(msgHeaderChanged(int)), 
	    this, SLOT(msgHeaderChanged(int)));
    connect(mFolder, SIGNAL(msgAdded(int)),
	    this, SLOT(msgAdded(int)));
    connect(mFolder, SIGNAL(msgRemoved(int)),
	    this, SLOT(msgRemoved(int)));
    connect(mFolder, SIGNAL(changed()),
	    this, SLOT(msgChanged()));
    connect(mFolder, SIGNAL(statusMsg(const char*)),
	    mOwner, SLOT(statusMsg(const char*)));
    readFolderConfig();
    mFolder->open();
  }

  updateMessageList();
}


//-----------------------------------------------------------------------------
void KMHeaders::msgChanged()
{
  int i = topItem();
  if (!autoUpdate()) return;
  updateMessageList();
  setTopItem(i);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgAdded(int id)
{
  if (!autoUpdate()) return;
  insertItem("", id);
  msgHeaderChanged(id);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgRemoved(int id)
{
  if (!autoUpdate()) return;
  removeItem(id);
}


//-----------------------------------------------------------------------------
void KMHeaders::msgHeaderChanged(int msgId)
{
  QString hdr(256);
  KMMsgStatus flag;
  KMMsgBase* mb;

  if (!autoUpdate()) return;

  mb = mFolder->getMsgBase(msgId);
  assert(mb != NULL);

  flag = mb->status();
  hdr.sprintf("%c\n%.100s\n %.100s\n%.40s", (char)flag, 
	      (const char*)KMMessage::stripEmailAddr(mb->from()),
	      (const char*)mb->subject(),
	      (const char*)mb->dateStr());
  changeItem(hdr, msgId);

  if (flag==KMMsgStatusNew) changeItemColor(darkRed, msgId);
  else if(flag==KMMsgStatusUnread) changeItemColor(darkBlue, msgId);
}


//-----------------------------------------------------------------------------
void KMHeaders::headerClicked(int column)
{
  int idx = currentItem();
  KMMsgBasePtr cur;
  const char* sortStr = "(unknown)";
  QString msg;

  if (idx >= 0) cur = (*mFolder)[idx];
  else cur = NULL;

  if (mSortCol == column) 
  {
    if (!mSortDescending) mSortDescending = TRUE;
    else
    {
      mSortCol = (int)KMMsgList::sfNone;
      mSortDescending = FALSE;
      sortStr = i18n("order of arrival");
    }
  }
  else 
  {
    mSortCol = column;
    mSortDescending = FALSE;
  }

  if (mSortCol==(int)KMMsgList::sfSubject) sortStr = i18n("subject");
  else if (mSortCol==(int)KMMsgList::sfDate) sortStr = i18n("date");
  else if (mSortCol==(int)KMMsgList::sfFrom) sortStr = i18n("sender");
  else if (mSortCol==(int)KMMsgList::sfStatus) sortStr = i18n("status");

  if (mSortDescending) msg.sprintf(i18n("Sorting messages descending by %s"), sortStr);
  else msg.sprintf(i18n("Sorting messages ascending by %s"), sortStr);
  mOwner->statusMsg(msg);

  kbp->busy();
  sort();
  kbp->idle();

  if (cur) idx = mFolder->find(cur);
  else idx = 0;

  setCurrentItem(idx);
  idx -= 3;
  if (idx < 0) idx = 0;
  setTopItem(idx);
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgStatus (KMMsgStatus status, int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
    msg->setStatus(status);
}


//-----------------------------------------------------------------------------
void KMHeaders::applyFiltersOnMsg(int msgId)
{
  KMMessage* msg;
  KMMessageList* msgList = selectedMsgs();
  int idx, cur = firstSelectedMsg(currentItem());

  for (idx=cur, msg=msgList->first(); msg; msg=msgList->next())
    filterMgr->process(msg);

  if (cur > count()) cur = count()-1;
  setCurrentItem(cur);
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  KMMessage* msg;
  KMMsgStatus st;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    st = msg->status();
    if (st==KMMsgStatusNew || st==KMMsgStatusUnread)
      msg->setStatus(KMMsgStatusOld);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  if (mFolder != trashFolder)
  {
    // move messages into trash folder
    moveMsgToFolder(trashFolder, msgId);
  }
  else
  {
    // We are in the trash folder -> really delete messages
    moveMsgToFolder(NULL, msgId);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::saveMsg (int msgId)
{
  KMMessage* msg;
  QString str;
  QString fileName = QFileDialog::getSaveFileName(".", "*");

  if (fileName.isEmpty()) return;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    str += "From ???@??? 00:00:00 1997 +0000\n";
    str += msg->asString();
    str += "\n";
  }

  if (kStringToFile(str, fileName, TRUE))
    mOwner->statusMsg(i18n("Message(s) saved."));
  else
    mOwner->statusMsg(i18n("Failed to save message(s)."));
}


//-----------------------------------------------------------------------------
void KMHeaders::resendMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg, *newMsg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  newMsg = new KMMessage;
  newMsg->fromString(msg->asString());
  newMsg->initHeader();
  newMsg->setTo(msg->to());
  newMsg->setSubject(msg->subject());

  win = new KMComposeWin;
  win->setMsg(newMsg, FALSE);
  win->show();
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createForward());
  win->show();
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createReply(FALSE));
  win->show();
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (int msgId)
{
  KMComposeWin *win;
  KMMessage *msg;

  msg = getMsg(msgId);
  if (!msg) return;

  kbp->busy();
  win = new KMComposeWin(msg->createReply(TRUE));
  win->show();
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::moveMsgToFolder (KMFolder* destFolder, int msgId)
{
  KMMessageList* msgList;
  KMMessage* msg;
  int top, rc, cur = firstSelectedMsg(currentItem());
  bool doUpd;

  kbp->busy();
  top = topItem();

  if (destFolder) destFolder->open();
  msgList = selectedMsgs(msgId);
  doUpd = (msgList->count() > 1);
  if (doUpd) setAutoUpdate(FALSE);
  for (rc=0, msg=msgList->first(); msg && !rc; msg=msgList->next())
  {
    if (destFolder) rc = destFolder->moveMsg(msg);
    else
    {
      if (!doUpd) removeItem(cur);
      mFolder->removeMsg(msg);
      delete msg;
    }
  }

  if (doUpd)
  {
    setAutoUpdate(TRUE);
    updateMessageList();
  }
  setCurrentMsg(cur);
  setTopItem(top);

  if (destFolder) destFolder->close();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::copyMsgToFolder (KMFolder* destFolder, int msgId)
{
  KMMessageList* msgList;
  KMMessage *msg, *newMsg;
  int top, rc;

  if (!destFolder) return;

  kbp->busy();
  top = topItem();

  destFolder->open();
  msgList = selectedMsgs(msgId);
  for (rc=0, msg=msgList->first(); msg && !rc; msg=msgList->next())
  {
    newMsg = new KMMessage;
    newMsg->fromString(msg->asString());
    assert(newMsg != NULL);

    destFolder->addMsg(newMsg);
  }
  destFolder->close();

  unmarkAll();
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::setCurrentMsg(int cur)
{
  if (cur >= mFolder->count()) cur = mFolder->count() - 1;
  setCurrentItem(cur, -1);
}


//-----------------------------------------------------------------------------
KMMessageList* KMHeaders::selectedMsgs(int idx)
{
  KMMessage* msg;

  mSelMsgList.clear();
  for (msg=getMsg(idx); msg; msg=getMsg())
    mSelMsgList.append(msg);

  return &mSelMsgList;
}


//-----------------------------------------------------------------------------
int KMHeaders::firstSelectedMsg (int msgId) const
{
  if (msgId<0 || msgId>=numRows()) return 0;
  while (msgId>=0 && itemList[msgId]->isMarked())
    msgId--;
  return (msgId+1);
}


//-----------------------------------------------------------------------------
KMMessage* KMHeaders::getMsg (int msgId)
{
  int i, high;

  if (!mFolder || msgId < -2)
  {
    getMsgIndex = -1;
    return NULL;
  }
  if (msgId >= 0) 
  {
    getMsgIndex = msgId;
    getMsgMulti = FALSE;
    return mFolder->getMsg(msgId);
  }

  if (msgId == -1)
  {
    getMsgMulti = TRUE;
    getMsgIndex = currentItem();
    for (i=0,high=numRows(); i<high; i++)
    {
      if (itemList[i]->isMarked())
      {
	getMsgIndex = i;
	break;
      }
    }
 
    return (getMsgIndex>=0 ? mFolder->getMsg(getMsgIndex) : (KMMessage*)NULL);
  }

  if (getMsgIndex < 0) return NULL;

  if (getMsgMulti)
  {
    for (getMsgIndex++; getMsgIndex<numRows(); getMsgIndex++)
    {
      if (itemList[getMsgIndex]->isMarked()) 
	return mFolder->getMsg(getMsgIndex);
    }
  }

  getMsgIndex = -1;
  return NULL;
}


//-----------------------------------------------------------------------------
void KMHeaders::nextMessage()
{
  int idx = currentItem();

  if (idx < mFolder->count())
    setCurrentItem(idx+1);
}


//-----------------------------------------------------------------------------
void KMHeaders::prevMessage()
{
  int idx = currentItem();

  if (idx > 0) 
    setCurrentItem(idx-1);
}  


//-----------------------------------------------------------------------------
void KMHeaders::changeItemPart (char c, int itemIndex, int column)
{
  char str[2];

  str[0] = c;
  str[1] = '\0';

  KMHeadersInherited::changeItemPart((const char*)str, itemIndex, column);
}


//-----------------------------------------------------------------------------
void KMHeaders::highlightMessage(int idx, int/*colId*/)
{
  if (idx < 0 || idx >= numRows()) return;

  kbp->busy();
  mOwner->statusMsg("");
  emit selected(mFolder->getMsg(idx));
  if (idx >= 0) setMsgRead(idx);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(int idx, int/*colId*/)
{
  if (idx < 0 || idx >= numRows()) return;

  kbp->busy();
  mOwner->statusMsg("");
  emit activated(mFolder->getMsg(idx));
  if (idx >= 0) setMsgRead(idx);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(void)
{
  long i;
  QString hdr(256);
  KMMsgStatus flag;
  KMMsgBase* mb;
 
  clear();
  if (!mFolder) return;

  kbp->busy();
  setAutoUpdate(FALSE);

  for (i=0; i<mFolder->count(); i++)
  {
    mb = mFolder->getMsgBase(i);
    assert(mb != NULL); // otherwise using count() above is wrong

    flag = mb->status();
    hdr.sprintf("%c\n%s\n %s\n%s", (char)flag, 
		(const char*)KMMessage::stripEmailAddr(mb->from()),
		(const char*)mb->subject(), (const char*)mb->dateStr());
    insertItem(hdr);

    if (flag==KMMsgStatusNew) changeItemColor(darkRed);
    else if(flag==KMMsgStatusUnread) changeItemColor(darkBlue);
  }

  setAutoUpdate(TRUE);
  repaint();

  hdr.sprintf(i18n("%d Messages, %d unread."),
	      mFolder->count(), mFolder->countUnread());
  if (mFolder->isReadOnly()) hdr += i18n("Folder is read-only.");

  mOwner->statusMsg(hdr);
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == RightButton)
  {
    if (mMouseCol >= 0) 
    {
      
    }
  }
  KMHeadersInherited::mouseReleaseEvent(e);
}


//-----------------------------------------------------------------------------
bool KMHeaders :: prepareForDrag (int /*aCol*/, int /*aRow*/, char** data, 
				  int* size, int* type)
{
  static KMDragData dd;
  int i, from, to, high;

  high = numRows()-1;
  for (i=0, from=-1; i<=high; i++)
  {
    if (itemList[i]->isMarked())
    {
      from = i;
      break;
    }
  }
  for (i=high, to=-1; i>=0; i--)
  {
    if (itemList[i]->isMarked())
    {
      to = i;
      break;
    }
  }
  if (from < 0 || to < 0) return FALSE;

  dd.init(mFolder, from, to);
  *data = (char*)&dd;
  *size = sizeof(dd);
  *type = DndRawData;

  debug("Ready to drag...");
  return TRUE;
}


//-----------------------------------------------------------------------------
void KMHeaders::sort(void)
{
  mFolder->sort((KMMsgList::SortField)mSortCol, mSortDescending);
}


//-----------------------------------------------------------------------------
#include "kmheaders.moc"
