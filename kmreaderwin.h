// Header for kmreaderwin the kmail reader
// written by Markus Wuebben <markus.wuebben@kde.org>

#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <qwidget.h>
#include <qdialog.h>
#include <qtimer.h>
#include <qcolor.h>
#include <qtextcodec.h>

class KHTMLPart;
class KMFolder;
class KMMessage;
class QFrame;
class QMultiLineEdit;
class QScrollBar;
class QString;
class QTabDialog;

namespace KParts
{
  struct URLArgs;
}

#define KMReaderWinInherited QWidget
class KMReaderWin: public QWidget
{
  Q_OBJECT

public:
  KMReaderWin(QWidget *parent=0, const char *name=0, int f=0);
  virtual ~KMReaderWin();

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
  //** Stype of attachments. */
  enum AttachmentStyle {IconicAttmnt=1, SmartAttmnt =2, InlineAttmnt = 3};

  /** Get/set the message header style. */
  HeaderStyle headerStyle(void) const { return mHeaderStyle; }
  virtual void setHeaderStyle(HeaderStyle style);

  /** Get/set the message attachment style. */
  AttachmentStyle attachmentStyle(void) const { return mAttachmentStyle;}
  virtual void setAttachmentStyle(int style);

  /** Get/set codec for reader win. */
  QTextCodec *codec(void) const { return mCodec; }
  virtual void setCodec(QTextCodec *codec);

  /** Set the message that shall be shown. If NULL, an empty page is
    displayed. */
  virtual void setMsg(KMMessage* msg, bool force = false);

  /** Returns the current message or NULL if none. */
  KMMessage* msg(void) const { return mMsg; }

  /** Clear the reader and discard the current message. */
  void clear(void) { setMsg(NULL); }

  /** Re-parse the current message. */
  void update(bool force = false) { setMsg(mMsg, force); }

  /** Print current message. */
  virtual void printMsg(void);

  /** Return selected text */
  QString copyText();

  /** Get/set auto-delete msg flag. */
  bool autoDelete(void) const { return mAutoDelete; }
  void setAutoDelete(bool f) { mAutoDelete=f; }

  /** Get/set message body font. */
  virtual void setBodyFont(const QString);
  const QString bodyFont(void) const { return mBodyFont; }

  /** Returns path where attachments are kept. Gets set with first
  KMReaderWin that is created. */
  static QString attachDir(void) { return mAttachDir; }

  /** Override default html mail setting */
  void setHtmlOverride( bool override );

  /** Is html mail to be supported? Takes into account override */
  bool htmlMail();

  /** View message part of type message/RFC822 in extra viewer window. */
  void atmViewMsg(KMMessagePart* msgPart);

  /** Display an attachment */
  static void atmView(KMReaderWin* aReaderWin, KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname, QTextCodec *codec);

signals:
  /** Emitted to show a text on the status line. */
  void statusMsg(const QString& text);

  /** The user presses the right mouse button. 'url' may be NULL. */
  void popupMenu(const KURL &url, const QPoint& mousePos);

  /** The user has clicked onto an URL that is no attachment. */
  void urlClicked(const KURL &url, int button);

  /** The user wants to see the attachment which is message */
  void showAtmMsg (KMMessage *msg);

public slots:

  /* Force update even if message is the same */
  void clearCache();

  /* Refresh the reader window */
  void updateReaderWin();

  /** HTML Widget scrollbar and layout handling. */
  void slotScrollUp();
  void slotScrollDown();
  void slotScrollPrior();
  void slotScrollNext();
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

protected slots:
  /** Some attachment operations. */
  void slotAtmOpen();
  void slotAtmOpenWith();
  void slotAtmView();
  void slotAtmPrint();
  void slotAtmSave();
  void slotAtmProperties();
  void slotDelayedResize();

protected:
  /** Watch for palette changes */
  virtual bool event(QEvent *e);

  /** Feeds the HTML viewer with the contents of the given message.
    HTML begin/end parts are written around the message. */
  virtual void parseMsg(void);

  /** Parse given message and add it's contents to the reader window. */
  virtual void parseMsg(KMMessage* msg);

  /** Creates a nice mail header depending on the current selected
    header style. */
  virtual void writeMsgHeader(int vcpartnum = -1);

  /** Feeds the HTML widget with the contents of the given message-body
    string. May contain body parts. */
  virtual void writeBodyStr(const QString bodyString);

  /** Create a nice icon with comment and name for the given
    body part, appended to the HTML view. Content type and subtype
    are set afterwards if they were not before. */
  virtual void writePartIcon(KMMessagePart* msgPart, int partNumber);

  /** Convert given string to HTML. Converts blanks and tabs at
    beginning of line to non-breakable spaces if preserveLeadingBlanks
    is TRUE. */
  virtual const QString strToHtml(const QString str,
				  bool decodeQuotedPrintable=TRUE,
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

 /** Create directory for attachments */
  virtual void makeAttachDir(void);

protected:
  QString colorToString(const QColor&);
  QString getAtmFilename(QString pname, QString msgpartname);

  bool mHtmlMail, mHtmlOverride;
  int mAtmInline;
  int mAtmCurrent;
  KMMessage *mMsg, *mMsgBuf;
  int mMsgBufSize;
  QString mMsgBufMD5;
  KHTMLPart *mViewer;
  HeaderStyle mHeaderStyle;
  AttachmentStyle mAttachmentStyle;
  bool mAutoDelete;
  QString mBodyFont;
  bool inlineImage;
  static QString mAttachDir;
  static const int delay;
  bool mBackingPixmapOn;
  QString mBackingPixmapStr;
  QTimer updateReaderWinTimer;
  QTimer mResizeTimer;
  QTextCodec *mCodec;
    bool mAutoDetectEncoding;

  int fntSize;
  QString mBodyFamily;
  QColor c1, c2, c3, c4;
  QString mQuoteFontTag[3];
  bool    mRecyleQouteColors;
};


#endif

