// $Id$

#include "kmcomposewin.h"
#include "kmheaders.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kbusyptr.h"
#include "kmdragdata.h"
#include "kmglobal.h"

#include <drag.h>
#include <qstrlist.h>
#include <klocale.h>


//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(QWidget *parent=0, const char *name=0) : KTabListBox(parent, name, 4)
{
  QString kdir = app->kdedir();

  folder = NULL;
  getMsgIndex = -1;

  //setNumCols(4);
  setColumn(0, nls->translate("F"), 16, KTabListBox::PixmapColumn);
  setColumn(1, nls->translate("Sender"), 150);
  setColumn(2, nls->translate("Subject"), 250);
  setColumn(3, nls->translate("Date"), 100);

  dict().insert(KMMessage::statusToStr(KMMessage::stNew),
		new QPixmap(kdir+"/lib/pics/kmmsgnew.xpm"));
  dict().insert(KMMessage::statusToStr(KMMessage::stUnread),
		new QPixmap(kdir+"/lib/pics/kmmsgunseen.xpm"));
  dict().insert(KMMessage::statusToStr(KMMessage::stDeleted),
		new QPixmap(kdir+"/lib/pics/kmmsgdel.xpm"));
  dict().insert(KMMessage::statusToStr(KMMessage::stOld),
		new QPixmap(kdir+"/lib/pics/kmmsgold.xpm"));

  connect(this,SIGNAL(selected(int,int)),
	  this,SLOT(selectMessage(int,int)));
  connect(this,SIGNAL(highlighted(int,int)),
	  this,SLOT(highlightMessage(int,int)));
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (KMFolder *f)
{
  if (folder) 
  {
    disconnect(folder, SIGNAL(msgHeaderChanged(int)),
	       this, SLOT(msgHeaderChanged(int)));
    disconnect(folder, SIGNAL(changed()),
	       this, SLOT(msgChanged()));
  }

  folder=f;

  if (folder)
  {
    connect(folder, SIGNAL(msgHeaderChanged(int)), 
	    this, SLOT(msgHeaderChanged(int)));
    connect(folder, SIGNAL(changed()),
	    this, SLOT(msgChanged()));
  }

  updateMessageList();
}


//-----------------------------------------------------------------------------
void KMHeaders::msgChanged()
{
  updateMessageList();
}


//-----------------------------------------------------------------------------
void KMHeaders::msgHeaderChanged(int msgId)
{
  char hdr[256];
  KMMessage* msg = folder->getMsg(msgId);
  KMMessage::Status flag;

  if (!msg) return;

  flag = msg->status();
  sprintf(hdr, "%c\n%s\n%s\n%s", (char)flag, msg->from(), msg->subject(), 
	  msg->dateStr());
  changeItem(hdr, msgId-1);

  if (flag==KMMessage::stNew) changeItemColor(darkRed, msgId-1);
  else if(flag==KMMessage::stUnread) changeItemColor(darkBlue, msgId-1);
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    msg->touch();
    //changeItemPart(msg->status(), getMsgIndex, 0);
    //changeItemColor(black, getMsgIndex);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {    
    msg->del();
    //changeItemPart(msg->status(), getMsgIndex, 0);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::undeleteMsg (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    msg->undel();
    //changeItemPart(msg->status(), getMsgIndex, 0);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::toggleDeleteMsg (int msgId)
{
  KMMessage* msg;

  if (!(msg=getMsg(msgId))) return;

  if (msg->status()==KMMessage::stDeleted) undeleteMsg(msgId);
  else deleteMsg(msgId);
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg (int msgId)
{
  KMComposeWin *w;
  KMMessage *msg = new KMMessage();

  kbp->busy();
  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    w = new KMComposeWin(0, 0, 0, msg, FORWARD);
    w->show();
  }
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (int msgId)
{
  KMComposeWin *w;
  KMMessage *msg = new KMMessage();

  kbp->busy();
  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    w = new KMComposeWin(0, 0, 0, msg, REPLY);
    w->show();
  }
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (int msgId)
{
  KMComposeWin *w;
  KMMessage *msg = new KMMessage();

  kbp->busy();
  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    w = new KMComposeWin(0, 0, 0, msg, REPLYALL);
    w->show();
  }
  kbp->idle(); 
}


//-----------------------------------------------------------------------------
void KMHeaders::moveMsgToFolder (KMFolder*/*destination*/, int /*msgId*/)
{
}


//-----------------------------------------------------------------------------
KMMessage* KMHeaders::getMsg (int msgId)
{
  int i, high;

  if (!folder || msgId < -2)
  {
    getMsgIndex = -1;
    return NULL;
  }
  if (msgId >= 0) 
  {
    getMsgIndex = msgId;
    getMsgMulti = FALSE;
    return folder->getMsg(msgId+1);
  }

  if (msgId == -1)
  {
    getMsgMulti = TRUE;
    getMsgIndex = currentItem();
    for (i=0,high=numRows(); i<high; i++)
    {
      if (itemList[i].isMarked())
      {
	getMsgIndex = i;
	break;
      }
    }
 
    return (getMsgIndex>=0 ? folder->getMsg(getMsgIndex+1) : (KMMessage*)NULL);
  }

  if (getMsgIndex < 0) return NULL;

  if (getMsgMulti) for (getMsgIndex++; getMsgIndex < numRows(); getMsgIndex++)
  {
    if (itemList[getMsgIndex].isMarked()) 
      return folder->getMsg(getMsgIndex+1);
  }

  getMsgIndex = -1;
  return NULL;
}


//-----------------------------------------------------------------------------
void KMHeaders::changeItemPart (char c, int itemIndex, int column)
{
  char str[2];

  str[0] = c;
  str[1] = '\0';

  KTabListBox::changeItemPart((const char*)str, itemIndex, column);
}


//-----------------------------------------------------------------------------
void KMHeaders::highlightMessage(int idx, int/*colId*/)
{
  kbp->busy();
  if (idx >= 0) setMsgRead(idx);
  
  emit messageSelected(folder->getMsg(idx+1));
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(int idx, int/*colId*/)
{
  KMMessage* msg;

  if (idx >= 0)
  {
    msg = getMsg(idx);
    if (msg->status()==KMMessage::stDeleted) undeleteMsg(idx);
    else deleteMsg(idx);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(void)
{
  long i;
  char hdr[256];
  KMMessage::Status flag;
  KMMessage* msg;

  clear();
  if (!folder) return;

  kbp->busy();
  setAutoUpdate(FALSE);

  for (i = 1; i <= folder->numMsgs(); i++)
  {
    msg  = folder->getMsg(i);
    flag = msg->status();
    sprintf(hdr, "%c\n%s\n%s\n%s", (char)flag, msg->from(), msg->subject(), 
    	    msg->dateStr());
    insertItem(hdr);

    if (flag==KMMessage::stNew) changeItemColor(darkRed);
    else if(flag==KMMessage::stUnread) changeItemColor(darkBlue);
  }

  setAutoUpdate(TRUE);
  repaint();
  kbp->idle();
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
    if (itemList[i].isMarked())
    {
      from = i;
      break;
    }
  }
  for (i=high-1, to=-1; i>=0; i--)
  {
    if (itemList[i].isMarked())
    {
      to = i;
      break;
    }
  }
  if (from < 0 || to < 0) return FALSE;

  dd.init(folder, from, to);
  *data = (char*)&dd;
  *size = sizeof(dd);
  *type = DndRawData;

  return TRUE;
}


//-----------------------------------------------------------------------------
#include "kmheaders.moc"
