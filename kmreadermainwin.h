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
class KMFolder;
template <typename T, typename S> class QMap;

class KMReaderMainWin : public KMail::SecondaryWindow
{
  Q_OBJECT

public:
  KMReaderMainWin( bool htmlOverride, bool htmlLoadExtOverride, char *name = 0 );
  KMReaderMainWin( char *name = 0 );
  KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QString & encoding, char *name = 0 );
  virtual ~KMReaderMainWin();

  void setUseFixedFont( bool useFixedFont );

  // take ownership of and show @param msg
  void showMsg( const QString & encoding, KMMessage *msg );

private slots:
  void slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint);

  /** Copy selected messages to folder with corresponding to given menuid */
  void copySelectedToFolder( int menuId );
  void slotTrashMsg();
  void slotPrintMsg();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotReplyAuthorToMsg();
  void slotReplyListToMsg();
  void slotForwardInlineMsg();
  void slotForwardAttachedMsg();
  void slotForwardDigestMsg();
  void slotRedirectMsg();
  void slotShowMsgSrc();
  void slotMarkAll();
  void slotCopy();
  void slotFind();
  void slotFindNext();

  void slotConfigChanged();

  void slotFolderRemoved( QObject* folderPtr );

private:
  void initKMReaderMainWin();
  void setupAccel();

  KMReaderWin *mReaderWin;
  KMMessage *mMsg;
  KURL mUrl;
  QMap<int,KMFolder*> mMenuToFolder;
  // a few actions duplicated from kmmainwidget
  KAction *mTrashAction, *mPrintAction, *mSaveAsAction, *mReplyAction,
          *mReplyAllAction, *mReplyAuthorAction, *mReplyListAction, *mForwardInlineAction,
          *mForwardAttachedAction, *mForwardDigestAction, *mRedirectAction,
          *mViewSourceAction;
  KActionMenu *mReplyActionMenu;
  KActionMenu *mForwardActionMenu;

};

#endif /*KMReaderMainWin_h*/
