#ifndef KMReaderMainWin_h
#define KMReaderMainWin_h

#include <kurl.h>
#include "kmtopwidget.h"

class QTextCodec;
class KMReaderWin;
class KMMessage;
class KMMessagePart;
template <typename T, typename S> class QMap;

class KMReaderMainWin : public KMTopLevelWidget
{
  Q_OBJECT

public:
  KMReaderMainWin( bool htmlOverride, char *name = 0 );
  KMReaderMainWin( char *name = 0 );
  KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QTextCodec *codec, char *name = 0 );
  virtual ~KMReaderMainWin();
  // take ownership of and show @param msg
  void showMsg( const QTextCodec *codec, KMMessage *msg );

private slots:
  void slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint);

  void slotMailtoCompose(); 
  void slotMailtoForward();
  void slotMailtoAddAddrBook();
  void slotMailtoOpenAddrBook();
  void slotUrlCopy();
  void slotUrlOpen();
  void slotUrlSave();
  void slotMailtoReply();
  
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotBounceMsg();

  void slotSubjectFilter();
  void slotFromFilter();
  void slotToFilter();
  void slotMailingListFilter();

  /** Copy selected messages to folder with corresponding to given menuid */
  void copySelectedToFolder( int menuId );

  void slotToggleFixedFont();
  void slotShowMsgSrc();
  void slotPrintMsg();
  void slotSaveMsg();

private:
  void setupAccel();

  KMReaderWin *mReaderWin;
  KMMessage *mMsg;
  const QTextCodec *mCodec;
  KURL mUrl;
  QMap<int,KMFolder*> mMenuToFolder;
};

#endif /*KMReaderMainWin_h*/
