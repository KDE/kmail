// Header for kmreaderwin the kmail reader
// written by Markus Wuebben <markus.wuebben@kde.org>

#ifndef KMREADERWIN_H
#define KMREADERWIN_H

#include <qwidget.h>
#include <qdialog.h>

class KHTMLWidget;
class KMFolder;
class KMMessage;
class QFrame;
class QMultiLineEdit;
class QScrollBar;
class QString;
class QTabDialog;

#define KMReaderWinInherited QWidget
class KMReaderWin: public QWidget
{
  Q_OBJECT

public:
  KMReaderWin(QWidget *parent=0, const char *name=0, int f=0);
  virtual ~KMReaderWin();

  /** Read settings from app's config file. */
  virtual void readConfig(void);

  /** Write settings to app's config file. Calls sync() if withSync is TRUE. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Get/set maximum lines of text for inline attachments. */
  int inlineAttach(void) const { return mAtmInline; }
  virtual void setInlineAttach(int maxLines);

  /** Style of the message header. */
  enum HeaderStyle { HdrFancy=1, HdrBrief=2, HdrStandard=3, HdrLong=4,
                     HdrAll=5 };

  /** Get/set the message header style. */
  HeaderStyle headerStyle(void) const { return mHeaderStyle; }
  virtual void setHeaderStyle(HeaderStyle style);

  /** Set the message that shall be shown. If NULL, an empty page is 
    displayed. */
  virtual void setMsg(KMMessage* msg);

  /** Returns the current message or NULL if none. */
  KMMessage* msg(void) const { return mMsg; }

  /** Clear the reader and discard the current message. */
  void clear(void) { setMsg(NULL); }

  /** Re-parse the current message. */
  void update(void) { setMsg(mMsg); }

  /** Print current message. */
  virtual void printMsg(void);
  
  /** Return selected text */
  QString copyText();

signals:
  /** Emitted to show a text on the status line. */
  void statusMsg(const char* text);

  /** The user presses the right mouse button. */
  void popupMenu(const QPoint& mousePos);
                         
  /** The user has clicked onto an URL that is no attachment. */
  void urlClicked(const char* url, int button);
                         
public slots:
  /** HTML Widget scrollbar and layout handling. */
  void slotScrollVert(int _y);
  void slotScrollHorz(int _x);
  void slotScrollUp();
  void slotScrollDown();
  void slotScrollPrior();
  void slotScrollNext();
  void slotDocumentChanged();
  void slotDocumentDone();

  /** An URL has been activate with a click. */
  void slotUrlOpen(const char* url, int button);

  /** The mouse has moved on or off an URL. */
  void slotUrlOn(const char* url);

  /** The user presses the right mouse button on an URL. */
  void slotUrlPopup(const char* url, const QPoint& mousePos);

  /** Some attachment operations. */
  void slotAtmOpen();
  void slotAtmPrint();
  void slotAtmSave();
  void slotAtmProperties();

protected:
  /** Feeds the HTML viewer with the contents of the current message. */
  virtual void parseMsg(void);

  /** Creates a nice mail header depending on the current selected
    header style. */
  virtual void writeMsgHeader(void);

  /** Feeds the HTML widget with the contents of the given message-body
    string. May contain body parts. */
  virtual void writeBodyStr(const QString bodyString);

  /** Create a nice icon with comment and name for the given
    body part, appended to the HTML view. Content type and subtype
    are set afterwards if they were not before. */
  virtual void writePartIcon(KMMessagePart* msgPart, int partNumber);

  /** Convert given string to HTML. */
  virtual const QString strToHtml(const QString str, 
				  bool decodeQuotedPrintable=TRUE) const;

  /** HTML initialization. */
  virtual void initHtmlWidget(void);

  /** Some necessary event handling. */
  virtual void closeEvent(QCloseEvent *);
  virtual void resizeEvent(QResizeEvent *);

  /** Returns id of message part from given URL or -1 if invalid. */
  virtual int msgPartFromUrl(const char* url);

protected:
  int mAtmInline;
  int mAtmCurrent;
  KMMessage *mMsg;
  KHTMLWidget *mViewer;
  QScrollBar *mSbVert, *mSbHorz;
  QString mPicsDir;
  HeaderStyle mHeaderStyle;
};


#endif

