#ifndef __KMHEADERS
#define __KMHEADERS

#include <qwidget.h>
#include <qstrlist.h>
#include "ktablistbox.h"
#include "mclass.h"

class Folder;

class KMHeaders : public KTabListBox {
  Q_OBJECT
public:
  KMHeaders(QWidget *parent=0, const char *name=0);
  
  virtual void setFolder(Folder *);
  Folder* currentFolder(void) { return folder; }

  virtual void setMsgUnread(int msgId=-1);
  virtual void setMsgRead(int msgId=-1);
  virtual void deleteMsg(int msgId=-1);
  virtual void undeleteMsg(int msgId=-1);
  virtual void forwardMsg(int msgId=-1);
  virtual void replyToMsg(int msgId=-1);
  virtual void replyAllToMsg(int msgId=-1);
  virtual void moveMsgToFolder(Folder* destination, int msgId=-1);
	// These methods process the message in the folder with the
	// given msgId, or if no msgId is given the (all) selected
        // message(s) is (are) processed.

  virtual void changeItem (char c, int itemIndex, int column);
	// Change part of the contents of a line

signals:
  virtual void messageSelected(Message *);

protected slots:
  void selectMessage(int msgId, int colId);
  void highlightMessage(int msgId, int colId);

protected:
  Message* getMsg (int msgId=-2);
	// Returns message with given id or current message if no
	// id is given. First call with msgId==-1 returns first
	// selected message, subsequent calls with no argument
	// return the following selected messages.

private:
  virtual void updateMessageList(void);
  Folder *folder;
  int getMsgIndex;
};

#endif

