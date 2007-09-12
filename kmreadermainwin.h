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
class KMFolder;
class KFontAction;
class KFontSizeAction;
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
  void slotMsgPopup(KMMessage &aMsg, const KUrl &aUrl, const QPoint& aPoint);

  /** Copy selected messages to folder with corresponding to given QAction */
  void copySelectedToFolder( QAction* );
  void slotPrintMsg();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotReplyAuthorToMsg();
  void slotReplyListToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotShowMsgSrc();
  void slotFontAction(const QString &);
  void slotSizeAction(int);
  void slotCreateTodo();

  void slotConfigChanged();

  void slotFolderRemoved( QObject* folderPtr );

private:
  void initKMReaderMainWin();
  void setupAccel();

  KMReaderWin *mReaderWin;
  KMMessage *mMsg;
  KUrl mUrl;
  QMap<QAction*,KMFolder*> mMenuToFolder;
  // a few actions duplicated from kmmainwidget
  KAction *mPrintAction, *mSaveAsAction, *mSaveAtmAction,
          *mReplyAction, *mReplyAllAction, *mReplyAuthorAction, *mReplyListAction,
          *mForwardAction, *mForwardAttachedAction, *mRedirectAction,
          *mViewSourceAction, *mCreateTodoAction;
  KActionMenu *mReplyActionMenu;
  KActionMenu *mForwardActionMenu;
  KFontAction *fontAction;
  KFontSizeAction *fontSizeAction;

};

#endif /*KMReaderMainWin_h*/
