// $Id$

#include "kmcomposewin.h"
#include "kmheaders.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kbusyptr.h"
#include "kmdragdata.h"
#include <drag.h>
#include <qstrlist.h>

#define MFL_DEL 'D'
#define MFL_NEW 'N'
#define MFL_UNREAD 'U'

extern KBusyPtr* kbp;


//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(QWidget *parent=0, const char *name=0) : KTabListBox(parent, name, 4)
{
  folder = NULL;
  getMsgIndex = -1;

  //setNumCols(4);
  setColumn(0, "F", 16, KTabListBox::PixmapColumn);
  setColumn(1, "Sender", 150);
  setColumn(2, "Subject", 250);
  setColumn(3, "Date", 100);

  dict().insert(KMMessage::statusToStr(KMMessage::stNew),
		new QPixmap("pics/kmmsgnew.xpm"));
  dict().insert(KMMessage::statusToStr(KMMessage::stUnread),
		new QPixmap("pics/kmmsgunseen.xpm"));
  dict().insert(KMMessage::statusToStr(KMMessage::stDeleted),
		new QPixmap("pics/kmmsgdel.xpm"));

  connect(this,SIGNAL(selected(int,int)),
	  this,SLOT(selectMessage(int,int)));
  connect(this,SIGNAL(highlighted(int,int)),
	  this,SLOT(highlightMessage(int,int)));
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (KMFolder *f)
{
  folder=f;
  updateMessageList();
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgUnread (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    //msg->clearStatus(F_SEEN);
    changeItem(msg->status(), getMsgIndex, 0);
    changeItemColor(darkBlue, getMsgIndex);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    //msg->setStatus(F_SEEN);
    changeItem(msg->status(), getMsgIndex, 0);
    changeItemColor(black, getMsgIndex);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {    
    //msg->del();
    changeItem(msg->status(), getMsgIndex, 0);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::undeleteMsg (int msgId)
{
  KMMessage* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    //msg->undel();
    changeItem(msg->status(), getMsgIndex, 0);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::toggleDeleteMsg (int msgId)
{
  KMMessage* msg;

  if (!(msg=getMsg(msgId))) return;

  if (msg->status() != MFL_DEL) deleteMsg(msgId);
  else undeleteMsg(msgId);
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
void KMHeaders::changeItem (char c, int itemIndex, int column)
{
  char str[2];

  str[0] = c;
  str[1] = '\0';

  KTabListBox::changeItem((const char*)str, itemIndex, column);
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
    if (msg->status()=='D') undeleteMsg(idx);
    else deleteMsg(idx);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(void)
{
  long i;
  char hdr[256];
  char flag;
  KMMessage* msg;

  clear();
  if (!folder) return;

  kbp->busy();
  setAutoUpdate(FALSE);
  for (i = 1; i <= folder->numMsgs(); i++)
  {
    msg  = folder->getMsg(i);
    flag = msg->status();
    sprintf(hdr, "%c\n%s\n%s\n%s", flag, msg->from(), msg->subject(), 
    	    msg->dateStr());
    insertItem(hdr);

    if (flag=='N') changeItemColor(darkRed);
    else if(flag=='U') changeItemColor(darkBlue);
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
