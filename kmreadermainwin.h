// -*- mode: C++; c-file-style: "gnu" -*-

#ifndef KMReaderMainWin_h
#define KMReaderMainWin_h

#include "secondarywindow.h"

#include <kurl.h>
#include <KMime/Message>

#include <boost/scoped_ptr.hpp>
#include <akonadi/item.h>
#include <akonadi/collection.h>
#include <QModelIndex>
class KMReaderWin;
class KAction;
class KFontAction;
class KFontSizeAction;
class KJob;
template <typename T, typename S> class QMap;

namespace KMail {
class MessageActions;
}

namespace KMime {
  class Message;
  class Content;
}
Q_DECLARE_METATYPE(QModelIndex)

class KMReaderMainWin : public KMail::SecondaryWindow
{
  Q_OBJECT

public:
  KMReaderMainWin( bool htmlOverride, bool htmlLoadExtOverride, char *name = 0 );
  KMReaderMainWin( char *name = 0 );
  KMReaderMainWin( KMime::Content* aMsgPart, bool aHTML, const QString &encoding, char *name = 0 );
  virtual ~KMReaderMainWin();

  void setUseFixedFont( bool useFixedFont );

  /**
   * take ownership of and show @param msg
   *
   * The last two parameters, serNumOfOriginalMessage and nodeIdOffset, are needed when @p msg
   * is derived from another message, e.g. the user views an encapsulated message in this window.
   * Then, the reader needs to know about that original message, so those to parameters are passed
   * onto setOriginalMsg() of KMReaderWin.
   */
  void showMessage( const QString & encoding, const Akonadi::Item &msg, const Akonadi::Collection & parentCollection = Akonadi::Collection());
  
  void showMessage( const QString & encoding, KMime::Message::Ptr message);
private slots:
  void slotMessagePopup(const Akonadi::Item& , const KUrl&, const KUrl &imageUrl, const QPoint& );
  void slotDelayedMessagePopup( KJob* );
  void slotTrashMsg();
  void slotForwardInlineMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotFontAction(const QString &);
  void slotSizeAction(int);
  void slotCustomReplyToMsg( const QString &tmpl );
  void slotCustomReplyAllToMsg( const QString &tmpl );
  void slotCustomForwardMsg( const QString &tmpl );
  
  void slotEditToolbars();
  void slotConfigChanged();
  void slotUpdateToolbars();

  /// This closes the window if the setting to close the window after replying or
  /// forwarding is set.
  void slotReplyOrForwardFinished();
  void slotCopyItem(QAction*);
  void slotCopyResult( KJob * job );
 
private:
  Akonadi::Collection parentCollection() const;
  void initKMReaderMainWin();
  void setupAccel();
  KAction *copyActionMenu();
  
  KMReaderWin *mReaderWin;
  Akonadi::Item mMsg;
  KUrl mUrl;
  // a few actions duplicated from kmmainwidget
  KAction *mTrashAction, *mPrintAction, *mSaveAsAction, *mSaveAtmAction,
    *mViewSourceAction;
  KFontAction *fontAction;
  KFontSizeAction *fontSizeAction;
  KMail::MessageActions *mMsgActions;
  Akonadi::Collection mParentCollection;

};

#endif /*KMReaderMainWin_h*/
