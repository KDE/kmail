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
class CustomTemplatesMenu;
template <typename T, typename S> class QMap;

namespace KMail {
class MessageActions;
}

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
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotShowMsgSrc();
  void slotFontAction(const QString &);
  void slotSizeAction(int);
  void slotCreateTodo();
  void slotCustomReplyToMsg( const QString &tmpl );
  void slotCustomReplyAllToMsg( const QString &tmpl );
  void slotCustomForwardMsg( const QString &tmpl );

  void slotEditToolbars();
  void slotConfigChanged();
  void slotUpdateToolbars();

  void slotFolderRemoved( QObject* folderPtr );

private:
  void initKMReaderMainWin();
  void setupAccel();
  void updateMessageMenu();
  void updateCustomTemplateMenus();

  KMReaderWin *mReaderWin;
  KMMessage *mMsg;
  KUrl mUrl;
  QMap<QAction*,KMFolder*> mMenuToFolder;
  // a few actions duplicated from kmmainwidget
  KAction *mTrashAction, *mPrintAction, *mSaveAsAction, *mSaveAtmAction,
          *mForwardAction, *mForwardAttachedAction, *mRedirectAction,
          *mViewSourceAction;
  KActionMenu *mForwardActionMenu;
  KActionMenu *mCopyActionMenu;
  KFontAction *fontAction;
  KFontSizeAction *fontSizeAction;
  KMail::MessageActions *mMsgActions;

  // Custom template actions menu
  CustomTemplatesMenu *mCustomTemplateMenus;
};

#endif /*KMReaderMainWin_h*/
