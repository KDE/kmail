#include <qstrlist.h>
#include "kmheaders.h"
#include "mclass.h"


//-----------------------------------------------------------------------------
KMHeaders::KMHeaders(QWidget *parent=0, const char *name=0) : KTabListBox(parent, name)
{
  folder = NULL;
  getMsgIndex = -1;

  setNumCols(4);
  setColumn(0, "F", 16, KTabListBox::PixmapColumn);
  setColumn(1, "Sender", 100);
  setColumn(2, "Subject", 200);
  setColumn(3, "Date", 100);

  dict().insert("N", new QPixmap("pics/kmmsgnew.xpm"));
  dict().insert("U", new QPixmap("pics/kmmsgunseen.xpm"));
  dict().insert("D", new QPixmap("pics/kmmsgdel.xpm"));

  connect(this,SIGNAL(selected(int,int)),
	  this,SLOT(selectMessage(int,int)));
  connect(this,SIGNAL(highlighted(int,int)),
	  this,SLOT(highlightMessage(int,int)));
}


//-----------------------------------------------------------------------------
void KMHeaders::setFolder (Folder *f)
{
  if (folder) 
  {
    folder->close();
    delete folder;
  }

  folder=f;
  updateMessageList();
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgUnread (int msgId)
{
  Message* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    msg->clearFlag(F_SEEN);
    changeItem(msg->getFlag(), getMsgIndex, 0);
    changeItemColor(darkBlue, getMsgIndex);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::setMsgRead (int msgId)
{
  Message* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    msg->setFlag(F_SEEN);
    changeItem(msg->getFlag(), getMsgIndex, 0);
    changeItemColor(black, getMsgIndex);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::deleteMsg (int msgId)
{
  Message* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    msg->del();
    changeItem(msg->getFlag(), getMsgIndex, 0);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::undeleteMsg (int msgId)
{
  Message* msg;

  for (msg=getMsg(msgId); msg; msg=getMsg())
  {
    msg->undel();
    changeItem(msg->getFlag(), getMsgIndex, 0);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::forwardMsg (int msgId)
{
}


//-----------------------------------------------------------------------------
void KMHeaders::replyToMsg (int msgId)
{
}


//-----------------------------------------------------------------------------
void KMHeaders::replyAllToMsg (int msgId)
{
}


//-----------------------------------------------------------------------------
void KMHeaders::moveMsgToFolder (Folder* destination, int msgId)
{
}


//-----------------------------------------------------------------------------
Message* KMHeaders::getMsg (int msgId)
{
  if (!folder) 
  {
    getMsgIndex = -1;
    return NULL;
  }
  if (msgId >= 0) 
  {
    getMsgIndex = msgId;
    return folder->getMsg(msgId+1);
  }

  if (msgId == -1)
  {
    getMsgIndex = currentItem();
    return (getMsgIndex>=0 ? folder->getMsg(getMsgIndex+1) : NULL);
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
void KMHeaders::highlightMessage(int idx, int colId)
{
  printf ("message %d highlighted (column %d)\n", idx, colId);
  if (idx >= 0) setMsgRead(idx);

  emit messageSelected(folder->getMsg(idx+1));
}


//-----------------------------------------------------------------------------
void KMHeaders::selectMessage(int idx, int colId)
{
  Message* msg;

  if (idx >= 0)
  {
    msg = getMsg(idx);
    if (msg->getFlag()=='D') undeleteMsg(idx);
    else deleteMsg(idx);
  }
}


//-----------------------------------------------------------------------------
void KMHeaders::updateMessageList(void)
{
  unsigned long i;
  char hdr[256];
  char sfrom[80];
  char ssubject[80];
  char sdate[32];
  char flags;
  Message* msg;

  clear();
  if (!folder) return;

  setAutoUpdate(FALSE);
  for (i = (long)1; i <= folder->numMsg(); i++)
  {
    msg = folder->getMsg(i);
    flags = msg->getFlag();
    msg->getFrom(sfrom);
    msg->getSubject(ssubject);
    msg->getDate(sdate);
    sprintf(hdr, "%c\t%s\t%s\t%s", flags, sfrom, ssubject, sdate);
    insertItem(hdr);
    //printf ("%s\n", hdr);
    if (flags=='N') changeItemColor(darkRed);
    else if(flags=='U') changeItemColor(darkBlue);
  }
  setAutoUpdate(TRUE);
  repaint();
}


//-----------------------------------------------------------------------------
#include "kmheaders.moc"
