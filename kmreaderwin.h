// Header for kmreaderwin the kmail reader -*- c++ -*-
// written by Markus Wuebben <markus.wuebben@kde.org>

#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <qwidget.h>
#include <qtimer.h>
#include <qcolor.h>
#include <qstringlist.h>
#include <kaction.h>
#include <kurl.h>
#include "kmmsgbase.h"
#include "kmmimeparttree.h" // Needed for friend declaration.
#include <cryptplugwrapper.h>
#include <cryptplugwrapperlist.h>

class QFrame;
class QHBox;
class QLabel;
class QListViewItem;
class QScrollBar;
class QString;
class QStringList;
class QTabDialog;
class QTextCodec;
class CryptPlugWrapperList;
class DwHeaders;
class DwMediaType;
class KActionCollection;
class KHTMLPart;
class KURL;
class KMFolder;
class KMMessage;
class KMMessagePart;
namespace KMail {
  class PartMetaData;
  class ObjectTreeParser;
  class AttachmentStrategy;
  class HtmlWriter;
  class KHtmlPartHtmlWriter;
};

class partNode; // might be removed when KMime is used instead of mimelib
                //                                      (khz, 29.11.2001)

class NewByteArray; // providing operator+ on a QByteArray (khz, 21.06.2002)

namespace KParts {
  struct URLArgs;
}

#define KMReaderWinInherited QWidget
class KMReaderWin: public QWidget
{
  Q_OBJECT

  friend void KMMimePartTree::itemClicked( QListViewItem* item );
  friend void KMMimePartTree::itemRightClicked( QListViewItem* item, const QPoint & );
  friend void KMMimePartTree::slotSaveAs();

  friend class KMail::ObjectTreeParser;
  friend class KMail::KHtmlPartHtmlWriter;

public:
  KMReaderWin( QWidget *parent,
	       QWidget *mainWindow,
	       KActionCollection *actionCollection,
	       KMMimePartTree* mimePartTree=0,
               int* showMIMETreeMode=0,
               const char *name=0,
	       int f=0 );
  virtual ~KMReaderWin();

  /** assign a KMMimePartTree to this KMReaderWin */
  virtual void setMimePartTree( KMMimePartTree* mimePartTree );

  /** tell KMReaderWin whether groupware functionality may be used */
  virtual void setUseGroupware( bool );
  
  /** Read color settings and set palette */
  virtual void readColorConfig(void);

  /** Read settings from app's config file. */
  virtual void readConfig(void);

  /** Write settings to app's config file. Calls sync() if withSync is TRUE. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Builds the font tag that will be used for quouted lines */
  QString quoteFontTag( int quoteLevel );

  /** Get/set maximum lines of text for inline attachments. */
  int inlineAttach(void) const { return mAtmInline; }
  virtual void setInlineAttach(int maxLines);

  /** Style of the message header. */
  enum HeaderStyle { HdrFancy=1, HdrBrief=2, HdrStandard=3, HdrLong=4,
                     HdrAll=5 };

  /** Get/set the message header style. */
  HeaderStyle headerStyle(void) const { return mHeaderStyle; }
  virtual void setHeaderStyle(HeaderStyle style);

  /** Get/set the message attachment strategy. */
  const KMail::AttachmentStrategy * attachmentStrategy() const {
    return mAttachmentStrategy;
  }
  void setAttachmentStrategy( const KMail::AttachmentStrategy * strategy );

  /** Get/set codec for reader win. */
  const QTextCodec *codec(void) const { return mCodec; }
  virtual void setCodec(const QTextCodec *codec);

  /** Set printing mode */
  virtual void setPrinting(bool enable) { mPrinting = enable; }

  /** Set the message that shall be shown. If msg is 0, an empty page is
      displayed. */
  virtual void setMsg(KMMessage* msg, bool force = false);

  /** Instead of settings a message to be shown sets a message part
      to be shown */
  virtual void setMsgPart( KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QTextCodec *codec );

  /** Show or hide the Mime Tree Viewer if configuration
      is set to smart mode.  */
  void showHideMimeTree( bool showIt );

  /** Store message id of last viewed message,
      normally no need to call this function directly,
      since correct value is set automatically in
      parseMsg(KMMessage* aMsg, bool onlyProcessHeaders). */
  void setIdOfLastViewedMessage( QString msgId )
    { mIdOfLastViewedMessage = msgId; }

  /** Specify whether message is to be shown completely or not.
      This is used to make sure message contains it's headers
      when displayed in separate Viewer window after double-click */
  void setShowCompleteMessage( bool showCompleteMessage )
    { mShowCompleteMessage = showCompleteMessage; }

  /** Clear the reader and discard the current message. */
  void clear(bool force = false) { setMsg(0, force); }

  /** Re-parse the current message. */
  void update(bool force = false);

  /** Print current message. */
  virtual void printMsg(void);

  /** Return selected text */
  QString copyText();

  /** Get/set auto-delete msg flag. */
  bool autoDelete(void) const { return mAutoDelete; }
  void setAutoDelete(bool f) { mAutoDelete=f; }

  /** Override default html mail setting */
  bool htmlOverride() const { return mHtmlOverride; }
  void setHtmlOverride( bool override );

  /** Is html mail to be supported? Takes into account override */
  bool htmlMail();

  /** Display the about page instead of a message */
  void displayAboutPage();

  /** Enable the displaying of messages again after an URL was displayed */
  void enableMsgDisplay() { mMsgDisplay = TRUE; }

  /** View message part of type message/RFC822 in extra viewer window. */
  void atmViewMsg(KMMessagePart* msgPart);

  bool atBottom() const;

  bool isfixedFont() { return mUseFixedFont; }

  /** Queue HTML code to be sent later in chunks to khtml */
  void queueHtml(const QString &aStr);

  /** Return a @ref HtmlWriter connected to the @ref KHTMLPart we use */
  KMail::HtmlWriter * makeHtmlWriter();

  // Action to reply to a message
  // but action( "some_name" ) some name could be used instead.
  KAction *replyAction() { return mReplyAction; }
  KAction *replyAllAction() { return mReplyAllAction; }
  KAction *replyListAction() { return mReplyListAction; }
  KActionMenu *forwardMenu() { return mForwardActionMenu; }
  KAction *forwardAction() { return mForwardAction; }
  KAction *forwardAttachedAction() { return mForwardAttachedAction; }
  KAction *redirectAction() { return mRedirectAction; }
  KAction *noQuoteReplyAction() { return mNoQuoteReplyAction; }

  KAction *bounceAction() { return mBounceAction; }

  KActionMenu *filterMenu() { return mFilterMenu; }
  KAction *subjectFilterAction() { return mSubjectFilterAction; }
  KAction *fromFilterAction() { return mFromFilterAction; }
  KAction *toFilterAction() { return mToFilterAction; }
  KAction *listFilterAction() { return mListFilterAction; }
  void updateListFilterAction();
  KToggleAction *toggleFixFontAction() { return mToggleFixFontAction; }
  KAction *viewSourceAction() { return mViewSourceAction; }
  KAction *printAction() { return mPrintAction; }
  KAction *mailToComposeAction() { return mMailToComposeAction; }
  KAction *mailToReplyAction() { return mMailToReplyAction; }
  KAction *mailToForwardAction() { return mMailToForwardAction; }
  KAction *addAddrBookAction() { return mAddAddrBookAction; }
  KAction *openAddrBookAction() { return mOpenAddrBookAction; }
  KAction *copyAction() { return mCopyAction; }
  KAction *urlOpenAction() { return mUrlOpenAction; }
  KAction *urlSaveAsAction() { return mUrlSaveAsAction; }

    // This function returns the complete data that were in this
    // message parts - *after* all encryption has been removed that
    // could be removed.
    // - This is used to store the message in decrypted form.
    void objectTreeToDecryptedMsg( partNode* node,
                                   NewByteArray& resultingData,
                                   KMMessage& theMessage,
                                   bool weAreReplacingTheRootNode = false,
                                   int recCount = 0 );

signals:
  /** Emitted after parsing of a message to have it stored
      in unencrypted state in it's folder. */
  void replaceMsgByUnencryptedVersion();

  /** Emitted to show a text on the status line. */
  void statusMsg(const QString& text);

  /** The user presses the right mouse button. 'url' may be 0. */
  void popupMenu(KMMessage &msg, const KURL &url, const QPoint& mousePos);

  /** The user has clicked onto an URL that is no attachment. */
  void urlClicked(const KURL &url, int button);

  /** Pgp displays a password dialog */
  void noDrag(void);

  /** Show/hide Groupware */
  void signalGroupwareShow(bool);

public slots:

  /** Select message body. */
  void selectAll();

  /** Force update even if message is the same */
  void clearCache();

  /** Refresh the reader window */
  void updateReaderWin();

  /** HTML Widget scrollbar and layout handling. */
  void slotScrollUp();
  void slotScrollDown();
  void slotScrollPrior();
  void slotScrollNext();
  void slotJumpDown();
  void slotDocumentChanged();
  void slotDocumentDone();
  void slotTextSelected(bool);

  /** An URL has been activate with a click. */
  void slotUrlOpen(const KURL &url, const KParts::URLArgs &args);

  /** The mouse has moved on or off an URL. */
  void slotUrlOn(const QString &url);

  /** The user presses the right mouse button on an URL. */
  void slotUrlPopup(const QString &, const QPoint& mousePos);

  /** The user selected "Find" from the menu. */
  void slotFind();

  /** The user toggled the "Fixed Font" flag from the view menu. */
  void slotToggleFixedFont();

  /** Copy the selected text to the clipboard */
  void slotCopySelectedText();

  /** Starts sending the queued HTML code to khtml */
  void sendNextHtmlChunk();

  /** Slot to reply to a message */
  void slotReplyToMsg();
  void slotReplyListToMsg();
  void slotReplyAllToMsg();
  void slotForward();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotRedirectMsg();
  void slotBounceMsg();
  void slotNoQuoteReplyToMsg();
  void slotSubjectFilter();
  void slotMailingListFilter();
  void slotFromFilter();
  void slotToFilter();
  void slotUrlClicked();

  /** Operations on mailto: URLs. */
  void slotMailtoReply();
  void slotMailtoCompose();
  void slotMailtoForward();
  void slotMailtoAddAddrBook();
  void slotMailtoOpenAddrBook();
  /** Copy URL in mUrlCurrent to clipboard. Removes "mailto:" at
      beginning of URL before copying. */
  void slotUrlCopy();
  /** Open URL in mUrlCurrent using Kfm. */
  void slotUrlOpen();
  /** Save the page to a file */
  void slotUrlSave();

  void slotShowMsgSrc();
  void slotPrintMsg();
  void slotSaveMsg();

protected slots:
  /** Returns the current message or 0 if none. */
  KMMessage* message(KMFolder** folder=0) const;

  /** Some attachment operations. */
  void slotAtmOpen();
  void slotAtmOpenWith();
  void slotAtmView();
  void slotAtmSave();
  void slotAtmProperties();
  void slotDelayedResize();
  void slotTouchMessage();

protected:
  /** Watch for palette changes */
  virtual bool event(QEvent *e);

  /** Calculate the pixel size */
  int pointsToPixel(int pointSize) const;

  /** Feeds the HTML viewer with the contents of the given message.
    HTML begin/end parts are written around the message. */
  virtual void parseMsg(void);

  /** write all headers that are not listed in mHideExtraHeaders[mHeaderStyle-1]
      to a string, using the formatString.
      @param aMsg The Message from which to extract the headers
      @param formatString Format String for a header, expected to
        take 3 Arguments:
        <ul>
          <li><b>%1</b>:&nbsp;Headerfield Name</li>
          <li><b>%2</b>:&nbsp;Translated Headerfield Name</li>
          <li><b>%3</b>:&nbsp;Headerfield Body</li>
        </ul>
      @param ignoreFieldsList contains the fieldnames that should not be displayed by this method
         (mostly used because those are displayed in the mail already)
  */
  QString visibleHeadersToString(KMMessage* aMsg, QString const &formatString, QStringList const &ignoreFieldsList);

  /** Parse given message and add it's contents to the reader window. */
  virtual void parseMsg(KMMessage* msg, bool onlyProcessHeaders=false);

  /** Creates a nice mail header depending on the current selected
    header style. */
  virtual QString writeMsgHeader(KMMessage* aMsg, bool hasVCard);

  /** Processed the txt strings and composed an appropriate
    HTML string that is stored into data for displaying it
    instead of the mail part's decrypted content.
    Also used these texts for displaying a Sorry message box.
    This method was implemented for being called
    by writeSignedMIME and by okDecryptMIME. */
  virtual void showMessageAndSetData( const QString& txt0,
                                      const QString& txt1,
                                      const QString& txt2a,
                                      const QString& txt2b,
                                      QCString& data );

  /** Feeds the HTML widget with the contents of the given message-body
    string. May contain body parts. */
  virtual void writeBodyStr( const QCString bodyString,
			     const QTextCodec *aCodec,
                             const QString& fromAddress,
                             bool* isSigned = 0,
                             bool* isEncrypted = 0 );

  /** Feeds the HTML widget with the contents of the given HTML message-body
    string. May contain body parts. */
  virtual void writeHTMLStr(const QString& aStr);

  /** Create a nice icon with comment and name for the given
    body part, appended to the HTML view. Content type and subtype
    are set afterwards if they were not before. */
  virtual void writePartIcon(KMMessagePart* msgPart, int partNumber,
    bool quiet = FALSE);

  /** Convert given string to HTML. Converts blanks and tabs at
    beginning of line to non-breakable spaces if preserveLeadingBlanks
    is TRUE. */
  virtual QString strToHtml(const QString &str,
                            bool preserveLeadingBlanks=FALSE) const;

  /** Change the string to `quoted' html (meaning, that the quoted
    part of the message get italized */
  QString quotedHTML(const QString& pos);

  /** HTML initialization. */
  virtual void initHtmlWidget(void);

  /** Some necessary event handling. */
  virtual void closeEvent(QCloseEvent *);
  virtual void resizeEvent(QResizeEvent *);

  /** Returns id of message part from given URL or -1 if invalid. */
  virtual int msgPartFromUrl(const KURL &url);

  /** Cleanup the attachment temp files */
  virtual void removeTempFiles();

private:
  /** extracted parts from writeBodyStr() */
  QString sigStatusToString(CryptPlugWrapper* cryptPlug,
                            int status_code,
                            CryptPlugWrapper::SigStatusFlags statusFlags,
                            int& frameColor,
                            bool& showKeyInfos);
  QString writeSigstatHeader(KMail::PartMetaData& part,
                             CryptPlugWrapper* cryptPlug,
                             const QString& fromAddress);
  QString writeSigstatFooter(KMail::PartMetaData& part);

protected:
  bool mUseGroupware;
  bool mHtmlMail, mHtmlOverride;
  int mAtmInline;
  int mAtmCurrent;
  QString mAtmCurrentName;
  KMMessage *mMessage;
  QHBox *mBox;
  QLabel *mColorBar;
  KHTMLPart *mViewer;
  HeaderStyle mHeaderStyle;
  bool mShowAllHeaders[HdrAll];
  QStringList mHeadersHide[HdrAll];
  QStringList mHeadersShow[HdrAll];
  const KMail::AttachmentStrategy * mAttachmentStrategy;
  bool mAutoDelete;
  QFont mBodyFont, mFixedFont;
  bool mInlineImage;
  static QString mAttachDir;
  /** where did the user save the attachment last time */
  QString mSaveAttachDir;
  static const int delay;
  bool mBackingPixmapOn;
  QString mBackingPixmapStr;
  QTimer updateReaderWinTimer;
  QTimer mResizeTimer;
  QTimer mHtmlTimer;
  QTimer mDelayedMarkTimer;
  QStringList mHtmlQueue;
  const QTextCodec *mCodec;
  bool mAutoDetectEncoding;
  bool mMsgDisplay;
  bool mDelayedMarkAsRead;
  unsigned long mLastSerNum;
  KMMsgStatus mLastStatus;

  int fntSize;
  bool mUseFixedFont;
  bool mPrinting;
  bool mIsFirstTextPart;
  QString mBodyFamily;
  QColor c1, c2, c3, c4;
  // colors for PGP (Frame, Header, Body)
  QColor cPgpOk1F, cPgpOk1H, cPgpOk1B,
         cPgpOk0F, cPgpOk0H, cPgpOk0B,
         cPgpWarnF, cPgpWarnH, cPgpWarnB,
         cPgpErrF, cPgpErrH, cPgpErrB,
         cPgpEncrF, cPgpEncrH, cPgpEncrB;
  // color of frame of warning preceeding the source of HTML messages
  QColor cHtmlWarning;
  // colors for colorbar
  QColor cCBnoHtmlB, cCBnoHtmlF,
         cCBisHtmlB, cCBisHtmlF;
  QString mQuoteFontTag[3];
  bool    mRecyleQouteColors;
  bool    mLoadExternal;
  bool mShowColorbar;
  bool mShowCompleteMessage;
  uint mDelayedMarkTimeout;
  QStringList mTempFiles;
  QStringList mTempDirs;
  KMMimePartTree* mMimePartTree;
  int* mShowMIMETreeMode;
  partNode* mRootNode;
  QString mIdOfLastViewedMessage;
  QWidget *mMainWindow;
  KActionCollection *mActionCollection;
  // Composition actions
  KAction *mReplyAction, *mReplyAllAction, *mReplyListAction,
      *mForwardAction, *mForwardAttachedAction, *mRedirectAction,
      *mBounceAction, *mNoQuoteReplyAction;
  KActionMenu *mForwardActionMenu;
  // Filter actions
  KActionMenu *mFilterMenu;
  KAction *mSubjectFilterAction, *mFromFilterAction, *mToFilterAction,
      *mListFilterAction, *mViewSourceAction, *mPrintAction,
      *mMailToComposeAction, *mMailToReplyAction, *mMailToForwardAction,
      *mAddAddrBookAction, *mOpenAddrBookAction, *mCopyAction,
      *mUrlOpenAction, *mUrlSaveAsAction;

  KToggleAction *mToggleFixFontAction;
  KURL mUrlClicked;

public:
  bool mDebugReaderCrypto;
};


#endif

