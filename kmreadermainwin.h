// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMReaderMainWin_h
#define KMReaderMainWin_h

#include "secondarywindow.h"

#include <kurl.h>

class KMReaderWin;
class KMMessage;
class KMMessagePart;
class KAction;
class KActionMenu;
class KMFolderIndex;
template <typename T, typename S> class QMap;

class KMReaderMainWin : public KMail::SecondaryWindow
{
  Q_OBJECT

public:
  KMReaderMainWin( bool htmlOverride, char *name = 0 );
  KMReaderMainWin( char *name = 0 );
  KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QString & encoding, char *name = 0 );
  virtual ~KMReaderMainWin();
  // take ownership of and show @param msg
  void showMsg( const QString & encoding, KMMessage *msg );

private slots:
  void slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint);

  /** Copy selected messages to folder with corresponding to given menuid */
  void copySelectedToFolder( int menuId );
  void slotPrintMsg();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotReplyAuthorToMsg();
  void slotReplyListToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotBounceMsg();

  void slotConfigChanged();

 protected slots:
  /** XML-GUI stuff */
  void slotUpdateToolbars();
  void slotEditKeys();
  void slotEditToolbars();

private:
  void setupAccel();

  KMReaderWin *mReaderWin;
  KMMessage *mMsg;
  KURL mUrl;
  QMap<int,KMFolder*> mMenuToFolder;
  // a few actions duplicated from kmmainwidget
  KAction *mPrintAction, *mReplyAction, *mReplyAllAction, *mReplyAuthorAction,
          *mReplyListAction, *mForwardAction,
          *mForwardAttachedAction, *mRedirectAction, *mBounceAction;
  KAction *mCopyMsgTextAction, *mSelectAllTextAction;
  KActionMenu *mReplyActionMenu;
  KActionMenu *mForwardActionMenu;

};

#endif /*KMReaderMainWin_h*/
