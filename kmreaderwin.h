// Header for kmreaderwin the kmail reader
// written by Markus Wuebben <markus.wuebben@kde.org>

#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <qwidget.h>
#include <qtimer.h>
#include <qcolor.h>
#include <qstringlist.h>
#include <cryptplugwrapper.h>
#include "kmmsgbase.h"
#include "kmmimeparttree.h" // Needed for friend declaration.

class KHTMLPart;
class KMFolder;
class KMMessage;
class KMMessagePart;
class KURL;
class QFrame;
class QMultiLineEdit;
class QScrollBar;
class QString;
class QTabDialog;
class QLabel;
class QHBox;
class QTextCodec;
class DwMediaType;
class CryptPlugWrapperList;
class KMMessagePart;
class KURL;
class QListViewItem;

class partNode; // might be removed when KMime is used instead of mimelib
                //                                      (khz, 29.11.2001)

class NewByteArray; // providing operator+ on a QByteArray (khz, 21.06.2002)
class DwHeaders;

namespace KParts
{
  struct URLArgs;
}

#define KMReaderWinInherited QWidget
class KMReaderWin: public QWidget
{
  Q_OBJECT

  friend void KMMimePartTree::itemClicked( QListViewItem* item );
  friend void KMMimePartTree::itemRightClicked( QListViewItem* item, const QPoint & );
  friend void KMMimePartTree::slotSaveAs();

public:
  KMReaderWin( KMMimePartTree* mimePartTree=0,
               int* showMIMETreeMode=0,
               QWidget *parent=0,
               const char *name=0,
               int f=0 );
  virtual ~KMReaderWin();

  /** assign a KMMimePartTree to this KMReaderWin */
  virtual void setMimePartTree( KMMimePartTree* mimePartTree );

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
  /** Style of attachments. */
  enum AttachmentStyle {IconicAttmnt=1, SmartAttmnt =2, InlineAttmnt = 3, HideAttmnt = 4};

  /** Get/set the message header style. */
  HeaderStyle headerStyle(void) const { return mHeaderStyle; }
  virtual void setHeaderStyle(HeaderStyle style);

  /** Get/set the message attachment style. */
  AttachmentStyle attachmentStyle(void) const { return mAttachmentStyle;}
  virtual void setAttachmentStyle(int style);

  /** Get/set codec for reader win. */
  QTextCodec *codec(void) const { return mCodec; }
  virtual void setCodec(QTextCodec *codec);

  /** Set printing mode */
  virtual void setPrinting(bool enable) { mPrinting = enable; }

  /** Set the message that shall be shown. If NULL, an empty page is
      displayed. */
  virtual void setMsg(KMMessage* msg, bool force = false);

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

  /** Select message body. */
  void selectAll();

  /** Return selected text */
  QString copyText();

  /** Get/set auto-delete msg flag. */
  bool autoDelete(void) const { return mAutoDelete; }
  void setAutoDelete(bool f) { mAutoDelete=f; }

  /** Override default html mail setting */
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


// static functions:

    /** Display an attachment */
    static void atmView(KMReaderWin* aReaderWin,
                        KMMessagePart* aMsgPart,
                        bool aHTML,
                        const QString& aFileName,
                        const QString& pname,
                        QTextCodec *codec);

    /** find a plugin matching a given libName */
    static bool foundMatchingCryptPlug( QString libName,
                                        CryptPlugWrapper** useThisCryptPlug_ref,
                                        QWidget* parent = 0,
                                        QString verboseName = "" );

    /** 1. Create a new partNode using 'content' data and Content-Description
            found in 'cntDesc'.
        2. Make this node the child of 'node'.
        3. Insert the respective entries in the Mime Tree Viewer.
        3. Parse the 'node' to display the content. */
    //  Function will be replaced once KMime is alive.
    static void insertAndParseNewChildNode( KMReaderWin* reader,
                                            QCString* resultString,
                                            CryptPlugWrapper*     useThisCryptPlug,
                                            partNode& node,
                                            const char* content,
                                            const char* cntDesc );
    /** Parse beginning at a given node and recursively parsing
        the children of that node and it's next sibling. */
    //  Function is called internally by "parseMsg(KMMessage* msg)"
    //  and it will be replaced once KMime is alive.
    static void parseObjectTree( KMReaderWin* reader,
                                 QCString* resultString,
                                 CryptPlugWrapper*     useThisCryptPlug,
                                 partNode* node,
                                 bool showOneMimePart=false,
                                 bool keepEncryptions=false,
                                 bool includeSignatures=true );

    // This function returns the complete data that were in this
    // message parts - *after* all encryption has been removed that
    // could be removed.
    // - This is used to store the message in decrypted form.
    void objectTreeToDecryptedMsg( partNode* node,
                                   NewByteArray& resultingData,
                                   KMMessage& theMessage,
                                   bool weAreReplacingTheRootNode = false,
                                   int recCount = 0 );

    /** if data is 0:
            Feeds the HTML widget with the contents of the opaque signed
            data found in partNode 'sign'.
        if data is set:
            Feeds the HTML widget with the contents of the given
            multipart/signed object.
        Signature is tested.  May contain body parts.

        Returns whether a signature was found or not: use this to
        find out if opaque data is signed or not. */
    static bool writeOpaqueOrMultipartSignedData( KMReaderWin* reader,
                                                  QCString* resultString,
                                                  CryptPlugWrapper* useThisCryptPlug,
                                                  partNode* data,
                                                  partNode& sign,
                                                  const QString& fromAddress,
                                                  bool doCheck = true,
                                                  QCString* cleartextData = 0,
                                                  struct CryptPlugWrapper::SignatureMetaData* paramSigMeta = 0,
                                                  bool hideErrors = false );

    /** Returns the contents of the given multipart/encrypted
        object. Data is decypted.  May contain body parts. */
    static bool okDecryptMIME( KMReaderWin* reader,
                               CryptPlugWrapper*     useThisCryptPlug,
                               partNode& data,
                               QCString& decryptedData,
                               bool& signatureFound,
                               struct CryptPlugWrapper::SignatureMetaData& sigMeta,
                               bool showWarning,
                               bool& passphraseError,
                               QString& aErrorText );

    /** Delete all KMReaderWin's that do not have a parent. */
    static void deleteAllStandaloneWindows();

signals:
  /** Emitted after parsing of a message to have it stored
      in unencrypted state in it's folder. */
  void replaceMsgByUnencryptedVersion();

  /** Emitted to show a text on the status line. */
  void statusMsg(const QString& text);

  /** The user presses the right mouse button. 'url' may be NULL. */
  void popupMenu(KMMessage &msg, const KURL &url, const QPoint& mousePos);

  /** The user has clicked onto an URL that is no attachment. */
  void urlClicked(const KURL &url, int button);

  /** The user wants to see the attachment which is message */
  void showAtmMsg (KMMessage *msg);

  /** Pgp displays a password dialog */
  void noDrag(void);

public slots:

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

  /** Starts sending the queued HTML code to khtml */
  void sendNextHtmlChunk();

protected slots:
  /** Some attachment operations. */
  void slotAtmOpen();
  void slotAtmOpenWith();
  void slotAtmView();
  void slotAtmSave();
  void slotAtmProperties();
  void slotDelayedResize();
  void slotTouchMessage();

protected:
  /** Returns the current message or NULL if none. */
  KMMessage* message(void) const;

  /** Watch for palette changes */
  virtual bool event(QEvent *e);

  /** Calculate the pixel size */
  int pointsToPixel(int pointSize) const;

  /** Feeds the HTML viewer with the contents of the given message.
    HTML begin/end parts are written around the message. */
  virtual void parseMsg(void);


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
  virtual void writeBodyStr( const QCString bodyString, QTextCodec *aCodec,
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
  class PartMetaData;
  QString sigStatusToString(CryptPlugWrapper* cryptPlug,
                            int status_code,
                            CryptPlugWrapper::SigStatusFlags statusFlags,
                            int& frameColor,
                            bool& showKeyInfos);
  QString writeSigstatHeader(PartMetaData& part,
                             CryptPlugWrapper* cryptPlug,
                             const QString& fromAddress);
  QString writeSigstatFooter(PartMetaData& part);

protected:
  bool mHtmlMail, mHtmlOverride;
  int mAtmInline;
  int mAtmCurrent;
  QString mAtmCurrentName;
  KMMessage *mMessage;
  QHBox *mBox;
  QLabel *mColorBar;
  KHTMLPart *mViewer;
  HeaderStyle mHeaderStyle;
  AttachmentStyle mAttachmentStyle;
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
  QTextCodec *mCodec;
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
  QColor cPgpOk1F, cPgpOk1H, cPgpOk1B, // colors for PGP (Frame, Header, Body)
         cPgpOk0F, cPgpOk0H, cPgpOk0B,
         cPgpWarnF, cPgpWarnH, cPgpWarnB,
         cPgpErrF, cPgpErrH, cPgpErrB,
         cPgpEncrF, cPgpEncrH, cPgpEncrB;
  QColor cCBpgp, cCBplain, cCBhtml; // colors for colorbar
  QString mQuoteFontTag[3];
  bool    mRecyleQouteColors;
  bool    mUnicodeFont;
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
  static QPtrList<KMReaderWin> mStandaloneWindows;
  
public:
  bool mDebugReaderCrypto;
};


#endif

