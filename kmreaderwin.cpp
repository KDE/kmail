// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <qclipboard.h>
//#include <qlayout.h>
#include <qhbox.h>
#include <qstyle.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kopenwith.h>
#include <kpgp.h>
#include <ktempfile.h>

// khtml headers
#include <khtml_part.h>
#include <khtmlview.h> // So that we can get rid of the frames
#include <dom/html_element.h>
#include <dom/html_block.h>


#include <mimelib/mimepp.h>

#include "kmversion.h"
#include "kmglobal.h"

#include "kbusyptr.h"
#include "kfileio.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsgpartdlg.h"
#include "kmreaderwin.h"

// for selection
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

// X headers...
#undef Never
#undef Always

//--- Sven's save attachments to /tmp start ---
#include <unistd.h>
#include <klocale.h>
#include <kstandarddirs.h>  // for access and getpid
//--- Sven's save attachments to /tmp end ---

// for the click on attachment stuff (dnaber):
#include <kuserprofile.h>

// Do the tmp stuff correctly - thanks to Harri Porten for
// reminding me (sven)

#include "vcard.h"
#include "kmdisplayvcard.h"
#include <kpopupmenu.h>
#include <qimage.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

const int KMReaderWin::delay = 150;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent, const char *aName, int aFlags)
  :KMReaderWinInherited(aParent, aName, aFlags | Qt::WDestructiveClose)
{

  mAutoDelete = false;
  mMsg = 0;
  mMsgBuf = 0;
  mMsgBufMD5 = "";
  mMsgBufSize = -1;
  mVcnum = -1;
  mMsgDisplay = true;
  mPrinting = false;

  initHtmlWidget();
  readConfig();
  mHtmlOverride = false;
  mUseFixedFont = false;

  connect( &updateReaderWinTimer, SIGNAL(timeout()),
  	   this, SLOT(updateReaderWin()) );
  connect( &mResizeTimer, SIGNAL(timeout()),
  	   this, SLOT(slotDelayedResize()) );

  mCodec = 0;
  mAutoDetectEncoding = true;
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  delete mViewer;  //hack to prevent segfault on exit
  if (mAutoDelete) delete mMsg;
  removeTempFiles();
}


//-----------------------------------------------------------------------------
void KMReaderWin::removeTempFiles()
{
  for (QStringList::Iterator it = mTempFiles.begin(); it != mTempFiles.end();
    it++)
  {
    QFile::remove(*it);
  }
  mTempFiles.clear();
  for (QStringList::Iterator it = mTempDirs.begin(); it != mTempDirs.end();
    it++)
  {
    QDir(*it).rmdir(*it);
  }
  mTempDirs.clear();
}


//-----------------------------------------------------------------------------
bool KMReaderWin::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     if (mMsg) {
       mMsg->readConfig();
       setMsg(mMsg, true); // Force update
     }
     return true;
  }
  return KMReaderWinInherited::event(e);
}



//-----------------------------------------------------------------------------
void KMReaderWin::readColorConfig(void)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Reader");

  c1 = QColor(kapp->palette().active().text());
  c2 = KGlobalSettings::linkColor();
  c3 = KGlobalSettings::visitedLinkColor();
  c4 = QColor(kapp->palette().active().base());

  // The default colors are also defined in configuredialog.cpp
  cPgpEncrH = QColor( 0x00, 0x80, 0xFF ); // light blue
  cPgpOk1H  = QColor( 0x40, 0xFF, 0x40 ); // light green
  cPgpOk0H  = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
  cPgpWarnH = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
  cPgpErrH  = QColor( 0xFF, 0x00, 0x00 ); // red
  cCBpgp   = QColor( 0x80, 0xFF, 0x80 ); // very light green
  cCBplain = QColor( 0xFF, 0xFF, 0x80 ); // very light yellow
  cCBhtml  = QColor( 0xFF, 0x40, 0x40 ); // light red

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    c1 = config->readColorEntry("ForegroundColor",&c1);
    c2 = config->readColorEntry("LinkColor",&c2);
    c3 = config->readColorEntry("FollowedColor",&c3);
    c4 = config->readColorEntry("BackgroundColor",&c4);
    cPgpEncrH = config->readColorEntry( "PGPMessageEncr", &cPgpEncrH );
    cPgpOk1H  = config->readColorEntry( "PGPMessageOkKeyOk", &cPgpOk1H );
    cPgpOk0H  = config->readColorEntry( "PGPMessageOkKeyBad", &cPgpOk0H );
    cPgpWarnH = config->readColorEntry( "PGPMessageWarn", &cPgpWarnH );
    cPgpErrH  = config->readColorEntry( "PGPMessageErr", &cPgpErrH );
    cCBpgp   = config->readColorEntry( "ColorbarPGP", &cCBpgp );
    cCBplain = config->readColorEntry( "ColorbarPlain", &cCBplain );
    cCBhtml  = config->readColorEntry( "ColorbarHTML", &cCBhtml );
  }
  
  // determine the frame and body color for PGP messages from the header color
  // if the header color equals the background color then the other colors are
  // also set to the background color (-> old style PGP message viewing)
  // else
  // the brightness of the frame is set to 4/5 of the brightness of the header
  // the saturation of the body is set to 1/8 of the saturation of the header
  int h,s,v;
  if ( cPgpOk1H == c4 )
  { // header color == background color?
    cPgpOk1F = c4;
    cPgpOk1B = c4;
  }
  else
  {
    cPgpOk1H.hsv( &h, &s, &v );
    cPgpOk1F.setHsv( h, s, v*4/5 );
    cPgpOk1B.setHsv( h, s/8, v );
  }
  if ( cPgpOk0H == c4 )
  { // header color == background color?
    cPgpOk0F = c4;
    cPgpOk0B = c4;
  }
  else
  {
    cPgpOk0H.hsv( &h, &s, &v );
    cPgpOk0F.setHsv( h, s, v*4/5 );
    cPgpOk0B.setHsv( h, s/8, v );
  }
  if ( cPgpWarnH == c4 )
  { // header color == background color?
    cPgpWarnF = c4;
    cPgpWarnB = c4;
  }
  else
  {
    cPgpWarnH.hsv( &h, &s, &v );
    cPgpWarnF.setHsv( h, s, v*4/5 );
    cPgpWarnB.setHsv( h, s/8, v );
  }
  if ( cPgpErrH == c4 )
  { // header color == background color?
    cPgpErrF = c4;
    cPgpErrB = c4;
  }
  else
  {
    cPgpErrH.hsv( &h, &s, &v );
    cPgpErrF.setHsv( h, s, v*4/5 );
    cPgpErrB.setHsv( h, s/8, v );
  }

  if ( cPgpEncrH == c4 )
  { // header color == background color?
    cPgpEncrF = c4;
    cPgpEncrB = c4;
  }
  else
  {
    cPgpEncrH.hsv( &h, &s, &v );
    cPgpEncrF.setHsv( h, s, v*4/5 );
    cPgpEncrB.setHsv( h, s/8, v );
  }

  mRecyleQouteColors = config->readBoolEntry( "RecycleQuoteColors", false );

  //
  // Prepare the quoted fonts
  //
  mQuoteFontTag[0] = quoteFontTag(0);
  mQuoteFontTag[1] = quoteFontTag(1);
  mQuoteFontTag[2] = quoteFontTag(2);
}

//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = kapp->config();
  QString encoding;
  
  { // block defines the lifetime of KConfigGroupSaver
  KConfigGroupSaver saver(config, "Pixmaps");
  mBackingPixmapOn = FALSE;
  mBackingPixmapStr = config->readEntry("Readerwin","");
  if (mBackingPixmapStr != "")
    mBackingPixmapOn = TRUE;
  }

  {
  KConfigGroupSaver saver(config, "Reader");
  mHtmlMail = config->readBoolEntry( "htmlMail", false );
  mAtmInline = config->readNumEntry("attach-inline", 100);
  mHeaderStyle = (HeaderStyle)config->readNumEntry("hdr-style", HdrFancy);
  mAttachmentStyle = (AttachmentStyle)config->readNumEntry("attmnt-style",
							   SmartAttmnt);
  mLoadExternal = config->readBoolEntry( "htmlLoadExternal", false );
  mViewer->setOnlyLocalReferences( !mLoadExternal );
  mShowColorbar = config->readBoolEntry( "showColorbar", false );
  }

  {
  KConfigGroupSaver saver(config, "Fonts");
  mUnicodeFont = config->readBoolEntry("unicodeFont",FALSE);
  mBodyFont = KGlobalSettings::generalFont();
  mFixedFont = KGlobalSettings::fixedFont();
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    mBodyFont = config->readFontEntry((mPrinting) ? "print-font" : "body-font",
      &mBodyFont);
    mFixedFont = config->readFontEntry("fixed-font", &mFixedFont);
    fntSize = mBodyFont.pointSize();
    mBodyFamily = mBodyFont.family();
  }
  else {
    setFont(KGlobalSettings::generalFont());
    fntSize = KGlobalSettings::generalFont().pointSize();
    mBodyFamily = KGlobalSettings::generalFont().family();
  }
  mViewer->setStandardFont(mBodyFamily);
  }

  readColorConfig();

  if (mMsg) {
    update();
    mMsg->readConfig();
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig(bool aWithSync)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Reader");
  config->writeEntry("attach-inline", mAtmInline);
  config->writeEntry("hdr-style", (int)mHeaderStyle);
  config->writeEntry("attmnt-style",(int)mAttachmentStyle);
  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
QString KMReaderWin::quoteFontTag( int quoteLevel )
{
  KConfig *config = kapp->config();

  QColor color;

  { // block defines the lifetime of KConfigGroupSaver
    KConfigGroupSaver saver(config, "Reader");
    if( config->readBoolEntry( "defaultColors", true ) == true )
    {
      color = QColor(kapp->palette().active().text());
    }
    else
    {
      if( quoteLevel == 0 ) {
	QColor defaultColor( 0x00, 0x80, 0x00 );
	color = config->readColorEntry( "QuotedText1", &defaultColor );
      } else if( quoteLevel == 1 ) {
	QColor defaultColor( 0x00, 0x70, 0x00 );
	color = config->readColorEntry( "QuotedText2", &defaultColor );
      } else if( quoteLevel == 2 ) {
	QColor defaultColor( 0x00, 0x60, 0x00 );
	color = config->readColorEntry( "QuotedText3", &defaultColor );
      } else
	color = QColor(kapp->palette().active().base());
    }
  }

  QFont font;
  {
    KConfigGroupSaver saver(config, "Fonts");
    if( config->readBoolEntry( "defaultFonts", true ) == true )
    {
      font = KGlobalSettings::generalFont();
      font.setItalic(true);
    }
    else
    {
      const QFont defaultFont = QFont("helvetica");
      if( quoteLevel == 0 )
	font  = config->readFontEntry( "quote1-font", &defaultFont );
      else if( quoteLevel == 1 )
	font  = config->readFontEntry( "quote2-font", &defaultFont );
      else if( quoteLevel == 2 )
	font  = config->readFontEntry( "quote3-font", &defaultFont );
      else
      {
	font = KGlobalSettings::generalFont();
	font.setItalic(true);
      }
    }
  }

  QString str;
  if (mPrinting) str = QString("<span style=\"color:#000000\">");
  else str = QString("<span style=\"color:%1\">").arg( color.name() );
  if( font.italic() ) { str += "<i>"; }
  if( font.bold() ) { str += "<b>"; }
  return( str );
}


//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  mBox = new QHBox(this);

  mColorBar = new QLabel(" ", mBox);
  mColorBar->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  mColorBar->setEraseColor( c4 );

  if ( !mShowColorbar )
    mColorBar->hide();


  mViewer = new KHTMLPart(mBox, "khtml");
  mViewer->widget()->setFocusPolicy(WheelFocus);
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->setPluginsEnabled(false);
  mViewer->setJScriptEnabled(false); // just make this explicit
  mViewer->setJavaEnabled(false);    // just make this explicit
  mViewer->setMetaRefreshEnabled(false);
  mViewer->setURLCursor(KCursor::handCursor());

  // Espen 2000-05-14: Getting rid of thick ugly frames
  mViewer->view()->setLineWidth(0);

  connect(mViewer->browserExtension(),
          SIGNAL(openURLRequest(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotUrlOpen(const KURL &, const KParts::URLArgs &)));
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotUrlOpen(const KURL &, const KParts::URLArgs &)));
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotUrlPopup(const QString &, const QPoint &)));
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHeaderStyle(KMReaderWin::HeaderStyle aHeaderStyle)
{
  mHeaderStyle = aHeaderStyle;
  update(true);
  writeConfig(true);   // added this so we can forward w/ full headers
}


//-----------------------------------------------------------------------------
void KMReaderWin::setAttachmentStyle(int aAttachmentStyle)
{
  mAttachmentStyle = (AttachmentStyle)aAttachmentStyle;
  update(true);
}

//-----------------------------------------------------------------------------
void KMReaderWin::setCodec(QTextCodec *codec)
{
  mCodec = codec;
  if(!codec) {
    mAutoDetectEncoding = true;
    update(true);
    return;
  }
  mAutoDetectEncoding = false;
  update(true);
}

//-----------------------------------------------------------------------------
void KMReaderWin::setInlineAttach(int aAtmInline)
{
  mAtmInline = aAtmInline;
  update(true);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMsg(KMMessage* aMsg, bool force)
{
  if (aMsg)
      kdDebug(5006) << "(" << aMsg->getMsgSerNum() << ") " << aMsg->subject() << " " << aMsg->fromStrip() << endl;

  // If not forced and there is aMsg and aMsg is same as mMsg then return
  //if (!force && aMsg && mMsg == aMsg)
  //  return;

  kdDebug(5006) << "Not equal" << endl;

  // connect to the updates if we have hancy headers
  
  mMsg = aMsg;
  if (mMsg)
  {
    mMsg->setCodec(mCodec);
    mMsg->setDecodeHTML(htmlMail());
  }

  // Avoid flicker, somewhat of a cludge
  if (force) {
    mMsgBuf = 0;
    updateReaderWin();

    // Work around a very strange QTimer bug that requires more investigation
    // double clicking too quickly on message headers was crashing kmail
    updateReaderWinTimer.stop();
  }
  else if (updateReaderWinTimer.isActive())
    updateReaderWinTimer.changeInterval( delay );
  else
    updateReaderWinTimer.start( 0, TRUE );
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  updateReaderWinTimer.stop();
  mMsg = 0;
}

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  I18N_NOOP("Maildir support"),
  I18N_NOOP("Distribution lists"),
  I18N_NOOP("SMTP authentication"),
  I18N_NOOP("SMTP over SSL/TLS"),
  I18N_NOOP("Pipelining for POP3 "
	    "(faster mail download on slow responding networks)"),
  I18N_NOOP("Various improvements for IMAP"),
  I18N_NOOP("On-demand downloading or deleting without downloading "
	    "of big mails on a POP3 server"),
  I18N_NOOP("Automatic configuration of POP3/IMAP/SMTP security features"),
  I18N_NOOP("Automatic encoding selection for outgoing mails"),
  I18N_NOOP("DIGEST-MD5 authentication"),
  I18N_NOOP("Per-identity sent-mail and drafts folders"),
  I18N_NOOP("Expiry of old messages"),
  I18N_NOOP("Hotkey to temporary switch to fixed width fonts"),
  I18N_NOOP("UTF-7 support"),
  I18N_NOOP("Automatic encryption using OpenPGP"),
  I18N_NOOP("Enhanced status reports for encrypted/signed messages"),
};
static const int numKMailNewFeatures =
  sizeof kmailNewFeatures / sizeof *kmailNewFeatures;


//-----------------------------------------------------------------------------
void KMReaderWin::displayAboutPage()
{
  mColorBar->hide();
  mMsgDisplay = FALSE;
  QString location = locate("data", "kmail/about/main.html");
  QString content = kFileToString(location);
  mViewer->begin(location);
  QString info =
    i18n("%1: KMail version; %2: help:// URL; %3: homepage URL; "
	 "%4: prior KMail version; %5: prior KDE version; "
	 "%6: generated list of new features; "
	 "%7: First-time user text (only shown on first start)"
	 "--- end of comment ---",
	 "<h2>Welcome to KMail %1</h2><p>KMail is the email client for the K "
	 "Desktop Environment. It is designed to be fully compatible with "
	 "Internet mailing standards including MIME, SMTP, POP3 and IMAP."
	 "</p>\n"
	 "<ul><li>KMail has many powerful features which are described in the "
	 "<a href=\"%2\">documentation</a></li>\n"
	 "<li>The <a href=\"%3\">KMail homepage</A> offers information about "
	 "new versions of KMail</li></ul>\n"
	 "<p>Some of the new features in this release of KMail include "
	 "(compared to KMail %4, which is part of KDE %5):</p>\n"
	 "<ul>\n%6</ul>\n"
	 "%7\n"
	 "<p>We hope that you will enjoy KMail.</p>\n"
	 "<p>Thank you,</p>\n"
	 "<p>&nbsp; &nbsp; The KMail Team</p>")
    .arg(KMAIL_VERSION) // KMail version
    .arg("help:/kmail/index.html") // KMail help:// URL
    .arg("http://kmail.kde.org/") // KMail homepage URL
    .arg("1.3").arg("2.2"); // prior KMail and KDE version

  QString featureItems;
  for ( int i = 0 ; i < numKMailNewFeatures ; i++ )
    featureItems += i18n("<li>%1</li>\n").arg( i18n( kmailNewFeatures[i] ) );

  info = info.arg( featureItems );

  if( kernel->firstStart() ) {
    info = info.arg( i18n("<p>Please take a moment to fill in the KMail "
			  "configuration panel at Settings-&gt;Configure "
			  "KMail.\n"
			  "You need to create at least a default identity and "
			  "an incoming as well as outgoing mail account."
			  "</p>\n") );
  } else {
    info = info.arg( QString::null );
  }
  mViewer->write(content.arg(fntSize).arg(info));
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
/*  if (mMsgBuf && mMsg &&
      !mMsg->msgIdMD5().isEmpty() &&
      (mMsgBufMD5 == mMsg->msgIdMD5()) &&
      ((unsigned)mMsgBufSize == mMsg->msgSize()))
    return; */

  mMsgBuf = mMsg;
  if (mMsgBuf) {
    mMsgBufMD5 = mMsgBuf->msgIdMD5();
    mMsgBufSize = mMsgBuf->msgSize();
  }
  else {
    mMsgBufMD5 = "";
    mMsgBufSize = -1;
  }

/*  if (mMsg && !mMsg->msgIdMD5().isEmpty())
    updateReaderWinTimer.start( delay, TRUE ); */

  if (!mMsgDisplay) return;

  mViewer->view()->setUpdatesEnabled( false );
  mViewer->view()->viewport()->setUpdatesEnabled( false );
  static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);
  if (mMsg)
  { 
    if ( mShowColorbar )
      mColorBar->show();
    else
      mColorBar->hide();
    parseMsg();
  }
  else
  {
    mColorBar->hide();
    mViewer->begin( KURL( "file:/" ) );
    mViewer->write("<html><body" +
		   QString(" bgcolor=\"#%1\"").arg(colorToString(c4)));

    if (mBackingPixmapOn)
      mViewer->write(" background=\"file://" + mBackingPixmapStr + "\"");
    mViewer->write("></body></html>");
    mViewer->end();
  }
  mViewer->view()->viewport()->setUpdatesEnabled( true );
  mViewer->view()->setUpdatesEnabled( true );
  mViewer->view()->viewport()->repaint( false );
}


//-----------------------------------------------------------------------------
QString KMReaderWin::colorToString(const QColor& c)
{
  return QString::number(0x1000000 +
			 (c.red() << 16) +
			 (c.green() << 8) +
			 c.blue(), 16 ).mid(1);
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(void)
{
  if(mMsg == NULL)
    return;

  QString bkgrdStr = "";
  if (mBackingPixmapOn)
    bkgrdStr = " background=\"file://" + mBackingPixmapStr + "\"";

  mViewer->begin( KURL( "file:/" ) );

  QString type = mMsg->typeStr().lower();

  if (mAutoDetectEncoding) {
    mCodec = 0;
    QCString encoding;
    if (type.find("text/") != -1)
      encoding = mMsg->charset();
    else if (type.find("multipart/") != -1) {
      if (mMsg->numBodyParts() > 0) {
        KMMessagePart msgPart;
        mMsg->bodyPart(0, &msgPart);
        encoding = msgPart.charset();
      }
    }
    if (encoding.isEmpty())
      encoding = QTextCodec::codecForLocale()->name();
    mCodec = KMMsgBase::codecForName(encoding);
  }

  if (!mCodec)
    mCodec = QTextCodec::codecForName("iso8859-1");
  mMsg->setCodec(mCodec);

  mViewer->write("<html><head><style type=\"text/css\">" +
    ((mPrinting) ? QString("body { font-family: \"%1\"; font-size: %2pt; }\n")
        .arg( mBodyFamily ).arg( fntSize )
      : QString("body { font-family: \"%1\"; font-size: %2pt; "
        "color: #%3; background-color: #%4; }\n")
        .arg( mBodyFamily ).arg( fntSize ).arg(colorToString(c1))
        .arg(colorToString(c4))) +
    ((mPrinting) ? QString("a { color: #000000; text-decoration: none; }")
      : QString("a { color: #%1; ").arg(colorToString(c2)) +
        "text-decoration: none; }" + // just playing
        QString( "table.encr { width: 100%; background-color: #%1; "
                 "border-width: 0px; }\n" )
        .arg( colorToString( cPgpEncrF ) ) +
        QString( "tr.encrH { background-color: #%1; "
                 "font-weight: bold; }\n" )
        .arg( colorToString( cPgpEncrH ) ) +
        QString( "tr.encrB { background-color: #%1; }\n" )
        .arg( colorToString( cPgpEncrB ) ) +
        QString( "table.signOkKeyOk { width: 100%; background-color: #%1; "
                 "border-width: 0px; }\n" )
        .arg( colorToString( cPgpOk1F ) ) +
        QString( "tr.signOkKeyOkH { background-color: #%1; "
                 "font-weight: bold; }\n" )
        .arg( colorToString( cPgpOk1H ) ) +
        QString( "tr.signOkKeyOkB { background-color: #%1; }\n" )
        .arg( colorToString( cPgpOk1B ) ) +
        QString( "table.signOkKeyBad { width: 100%; background-color: #%1; "
                 "border-width: 0px; }\n" )
        .arg( colorToString( cPgpOk0F ) ) +
        QString( "tr.signOkKeyBadH { background-color: #%1; "
                 "font-weight: bold; }\n" )
        .arg( colorToString( cPgpOk0H ) ) +
        QString( "tr.signOkKeyBadB { background-color: #%1; }\n" )
        .arg( colorToString( cPgpOk0B ) ) +
        QString( "table.signWarn { width: 100%; background-color: #%1; "
                 "border-width: 0px; }\n" )
        .arg( colorToString( cPgpWarnF ) ) +
        QString( "tr.signWarnH { background-color: #%1; "
                 "font-weight: bold; }\n" )
        .arg( colorToString( cPgpWarnH ) ) +
        QString( "tr.signWarnB { background-color: #%1; }\n" )
        .arg( colorToString( cPgpWarnB ) ) +
        QString( "table.signErr { width: 100%; background-color: #%1; "
                 "border-width: 0px; }\n" )
        .arg( colorToString( cPgpErrF ) ) +
        QString( "tr.signErrH { background-color: #%1; "
                 "font-weight: bold; }\n" )
        .arg( colorToString( cPgpErrH ) ) +
        QString( "tr.signErrB { background-color: #%1; }\n" )
        .arg( colorToString( cPgpErrB ) )) +
		 "</style></head>" +
		 // TODO: move these to stylesheet, too:
    ((mPrinting) ? QString("<body>") : QString("<body ") + bkgrdStr + ">" ));

  if (!parent())
    setCaption(mMsg->subject());

  parseMsg(mMsg);

  mViewer->write("</body></html>");
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
  removeTempFiles();
  KMMessagePart msgPart;
  int i, numParts;
  QCString type, subtype, contDisp;
  QByteArray str;
  bool asIcon = false;
  inlineImage = false;
  VCard *vc;

  assert(aMsg!=NULL);
  type = aMsg->typeStr();
  numParts = aMsg->numBodyParts();

  // Hrm we have to iterate this twice with the current design.  This
  // should really be fixed.  (FIXME)
  mVcnum = -1;
  for (int j = 0; j < aMsg->numBodyParts(); j++) {
    aMsg->bodyPart(j, &msgPart);
    if (!qstricmp(msgPart.typeStr(), "text")
       && !qstricmp(msgPart.subtypeStr(), "x-vcard")) {
        int vcerr;
        QTextCodec *atmCodec = (mAutoDetectEncoding) ?
          KMMsgBase::codecForName(msgPart.charset()) : mCodec;
        if (!atmCodec) atmCodec = mCodec;
        vc = VCard::parseVCard(atmCodec->toUnicode(msgPart
          .bodyDecoded()), &vcerr);

        if (vc) {
          delete vc;
          kdDebug(5006) << "FOUND A VALID VCARD" << endl;
          mVcnum = j;
          writePartIcon(&msgPart, j, TRUE);
          break;
        }
    }
  }
  
  mViewer->write("<div name=\"header\" style=\"margin-bottom: 1em;\">" + (writeMsgHeader()) + "</div>");

  if (numParts > 0)
  {
    // ---sven: handle multipart/alternative start ---
    // This is for multipart/alternative messages WITHOUT attachments
    // main header has type=multipart/alternative and one attachment is
    // text/html
    if (type.find("multipart/alternative") != -1 && numParts == 2)
    {
      kdDebug(5006) << "Alternative message, type: " << type << endl;
      //Now: Only two attachments one of them is html
      for (i=0; i<2; i++)                   // count parts...
      {
        aMsg->bodyPart(i, &msgPart);        // set part...
        subtype = msgPart.subtypeStr();     // get subtype...
        if (htmlMail() && qstricmp(subtype, "html")==0)    // is it html?
        {                                   // yes...
          str = msgPart.bodyDecoded();      // decode it...
          //mViewer->write(mCodec->toUnicode(str.data()));    // write it...
          writeHTMLStr(mCodec->toUnicode(str.data()));
          return;                           // return, finshed.
        }
	else if (!htmlMail() && (qstricmp(subtype, "plain")==0))
	                                    // wasn't html show only if
	{                                   // support for html is turned off
          str = msgPart.bodyDecoded();      // decode it...
          writeBodyStr(str.data(), mCodec);
          return;
	}
      }
      // if we are here we didnt find any html part. Handle it normaly then
    }
    // This works only for alternative msgs without attachments. Alternative
    // messages with attachments are broken with or without this. No need
    // to bother with strib </body> or </html> here, because if any part
    // follows this will not be shown correctly. You'll still be able to read the
    // main message and deal with attachments. Nothing I can do now :-(
    // ---sven: handle multipart/alternative end ---

    for (i=0; i<numParts; i++)
    {
      aMsg->bodyPart(i, &msgPart);
      type = msgPart.typeStr();
      subtype = msgPart.subtypeStr();
      contDisp = msgPart.contentDisposition();
      kdDebug(5006) << "type: " << type.data() << endl;
      kdDebug(5006) << "subtye: " << subtype.data() << endl;
      kdDebug(5006) << "contDisp " << contDisp.data() << endl;

      if (i <= 0) asIcon = FALSE;
      else switch (mAttachmentStyle)
      {
      case IconicAttmnt:
        asIcon=TRUE; break;
      case InlineAttmnt:
        asIcon=FALSE; break;
      case SmartAttmnt:
        asIcon=(contDisp.find("inline")<0);
      }

      if (!asIcon)
      {
//	if (i<=0 || qstricmp(type, "text")==0)//||qstricmp(type, "message")==0)
//	if (qstricmp(type, "text")==0)//||qstricmp(type, "message")==0)
	if ((type == "") || (qstricmp(type, "text")==0))
	{
          QCString cstr(msgPart.bodyDecoded());
	  if (i>0)
	      mViewer->write("<br><hr><br>");

          QTextCodec *atmCodec = (mAutoDetectEncoding) ?
            KMMsgBase::codecForName(msgPart.charset()) : mCodec;
          if (!atmCodec) atmCodec = mCodec;
          
	  if (htmlMail() && (qstricmp(subtype, "html")==0))
          {
            // ---Sven's strip </BODY> and </HTML> from end of attachment start-
            // We must fo this, or else we will see only 1st inlined html attachment
            // It is IMHO enough to search only for </BODY> and put \0 there.
            int i;
            i = cstr.findRev("</body>", -1, false); //case insensitive
            if (i>0)
              cstr.truncate(i);
            else // just in case - search for </html>
            {
              i = cstr.findRev("</html>", -1, false); //case insensitive
              if (i>0) cstr.truncate(i);
            }
            // ---Sven's strip </BODY> and </HTML> from end of attachment end-
            //mViewer->write(atmCodec->toUnicode(cstr.data()));
            writeHTMLStr(atmCodec->toUnicode(cstr.data()));
	  }
          else writeBodyStr(cstr, atmCodec);
	}
        // ---Sven's view smart or inline image attachments in kmail start---
        else if (qstricmp(type, "image")==0)
        {
          inlineImage=true;
          writePartIcon(&msgPart, i);
          inlineImage=false;
        }
        // ---Sven's view smart or inline image attachments in kmail end---
	else asIcon = TRUE;
      }
      if (asIcon)
      {
        writePartIcon(&msgPart, i);
      }
    }
  }
  else // if numBodyParts <= 0
  {
    if (htmlMail() && ((type == "text/html") || (type.find("text/html") != -1)))
      //mViewer->write(mCodec->toUnicode(aMsg->bodyDecoded().data()));
      writeHTMLStr(mCodec->toUnicode(aMsg->bodyDecoded().data()));
    else
      writeBodyStr(aMsg->bodyDecoded(), mCodec);
  }
}


//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader()
{
  QString headerStr;
  QString vcname;

  if (mVcnum >= 0) vcname = mTempFiles.last();

  switch (mHeaderStyle)
  {
  case HdrBrief:
    headerStr.append("<b style=\"font-size:130%\">" + strToHtml(mMsg->subject()) +
                     "</b>&nbsp; (" +
                     KMMessage::emailAddrAsAnchor(mMsg->from(),TRUE) + ", ");
    
    if (!mMsg->cc().isEmpty())
    {
      headerStr.append(i18n("Cc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->cc(),TRUE) + ", ");
    }

    headerStr.append("&nbsp;"+strToHtml(mMsg->dateShortStr()) + ")");

    if (mVcnum >= 0)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }

    headerStr.append("<br>\n");
    break;

  case HdrStandard:
    headerStr.append("<b style=\"font-size:130%\">" +
                     strToHtml(mMsg->subject()) + "</b><br>\n");
    headerStr.append(i18n("From: ") +
                     KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (mVcnum >= 0) 
    {
      headerStr.append("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    headerStr.append("<br>\n");
    headerStr.append(i18n("To: ") +
                     KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>\n");
    if (!mMsg->cc().isEmpty())
      headerStr.append(i18n("Cc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>\n");
    break;

  case HdrFancy:
  {
    // prep our colours as rgb tripletts for use in the CSS
    QColorGroup cg = kapp->palette().active();
    QString foreground = QString("rgb(%1,%2,%3)")
                                .arg(cg.foreground().red())
                                .arg(cg.foreground().green())
                                .arg(cg.foreground().blue());
    QString highlight = QString("rgb(%1,%2,%3)")
                                .arg(cg.highlight().red())
                                .arg(cg.highlight().green())
                                .arg(cg.highlight().blue());
    QString highlightedText = QString("rgb(%1,%2,%3)")
                                .arg(cg.highlightedText().red())
                                .arg(cg.highlightedText().green())
                                .arg(cg.highlightedText().blue());
    QString background = QString("rgb(%1,%2,%3)")
                                .arg(cg.background().red())
                                .arg(cg.background().green())
                                .arg(cg.background().blue());


    // the subject line
    headerStr = QString("<div name=\"foo\" style=\"background: %1; "
                        "color: %2; padding: 4px; "
                        "border: solid %3 1px;\">"
                        "<b>%5</b></div>")
                       .arg((mPrinting) ? background : highlight)
                       .arg((mPrinting) ? foreground : highlightedText)
                       .arg(foreground)
                       .arg(strToHtml(mMsg->subject()));

    // the box with details below the subject
    headerStr.append(QString("<div style=\"background: %1; color: %2; "
                             "border-bottom: solid %3 1px; "
                             "border-left: solid %4 1px; "
                             "border-right: solid %5 1px; "
		                         "margin-bottom: 1em; "
                             "padding: 2px;\">"
                             "<table cellspacing=\"0\" cellpadding=\"4\">" )
                            .arg(background)
                            .arg(foreground)
                            .arg(foreground)
                            .arg(foreground)
                            .arg(foreground));

    // from line
    headerStr.append(QString("<tr><th valign=\"top\" align=\"left\">%1</th><td valign=\"top\">%2%3%4</td</tr>")
                            .arg(i18n("From: "))
                            .arg(KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE))
                            .arg((mVcnum < 0) ?
                                 ""
                                 : "&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>")
                            .arg((mMsg->headerField("Organization").isEmpty()) ?
                                 ""
                                 : "&nbsp;&nbsp;(" +
                                   strToHtml(mMsg->headerField("Organization")) +
                                   ")"));

    // to line
    headerStr.append(QString("<tr><th valign=\"top\" align=\"left\">%1</th><td valign=\"top\">%2</td</tr>")
                            .arg(i18n("To: "))
                            .arg(KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE)));

    // cc line, if any
    if (!mMsg->cc().isEmpty())
    {
      headerStr.append(QString("<tr><th valign=\"top\" align=\"left\">%1</th><td valign=\"top\">%2</td</tr>")
                              .arg(i18n("Cc: "))
                              .arg(KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE)));
    }

    // the date
    headerStr.append(QString("<tr><th valign=\"top\" align=\"left\">%1</th><td valign=\"top\">%2</td</tr>")
                            .arg(i18n("Date: "))
                            .arg(strToHtml(mMsg->dateStr())));
    headerStr.append("</table></div>");
    break;
  }
  case HdrLong:
    headerStr.append("<b style=\"font-size:130%\">" +
                     strToHtml(mMsg->subject()) + "</b><br>");
    headerStr.append(i18n("Date: ") + strToHtml(mMsg->dateStr())+"<br>");
    headerStr.append(i18n("From: ") +
                     KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (mVcnum >= 0)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\"" +
                       vcname + 
                       "\">"+i18n("[vCard]")+"</a>");
    }

    if (!mMsg->headerField("Organization").isEmpty())
    {
      headerStr.append("&nbsp;&nbsp;(" +
                       strToHtml(mMsg->headerField("Organization")) + ")");
    }

    headerStr.append("<br>\n");
    headerStr.append(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>");
    if (!mMsg->cc().isEmpty())
    {
      headerStr.append(i18n("Cc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>");
    }

    if (!mMsg->bcc().isEmpty())
    {
      headerStr.append(i18n("Bcc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->bcc(),FALSE) + "<br>");
    }
    
    if (!mMsg->replyTo().isEmpty())
    {
      headerStr.append(i18n("Reply to: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->replyTo(),FALSE) + "<br>");
    }
    break;

  case HdrAll:
    headerStr = strToHtml(mMsg->headerAsString(), true);
    headerStr.append("\n");
    if (mVcnum >= 0) 
    {
      headerStr.append("\n<br>\n<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    break;

  default:
    kdDebug(5006) << "Unsupported header style " << mHeaderStyle << endl;
  }

  return headerStr;
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr(const QCString aStr, QTextCodec *aCodec)
{
  QString line, htmlStr;
  QString signClass;
  bool goodSignature = false;
  Kpgp::Module* pgp = Kpgp::Module::getKpgp();
  assert(pgp != NULL);
  // assert(!aStr.isNull());
  //bool pgpMessage = false;
  bool isEncrypted = false, isSigned = false;

  if (pgp->setMessage(aStr))
  {
    QString signer;
    QCString str = pgp->frontmatter();
    if(!str.isEmpty()) htmlStr += quotedHTML(aCodec->toUnicode(str));
    //htmlStr += "<br>";
    isEncrypted = pgp->isEncrypted();
    if (isEncrypted)
    {
      emit noDrag();
      htmlStr += "<table cellspacing=\"1\" cellpading=\"0\" class=\"encr\">\n"
                 "<tr class=\"encrH\"><td>\n";
      if(pgp->decrypt())
      {
	htmlStr += i18n("Encrypted message");
      }
      else
      {
	htmlStr += QString("%1<br />%2")
                    .arg(i18n("Cannot decrypt message:"))
                    .arg(pgp->lastErrorMsg());
      }
      htmlStr += "</td></tr>\n<tr class=\"encrB\"><td>\n";
    }
    // check for PGP signing
    isSigned = pgp->isSigned();
    if (isSigned)
    {
      signer = pgp->signedBy();
      if (signer.isEmpty())
      {
        signClass = "signWarn";
        htmlStr += "<table cellspacing=\"1\" cellpading=\"0\" "
                   "class=\"" + signClass + "\">\n"
                   "<tr class=\"" + signClass + "H\"><td>\n";
        htmlStr += i18n("Message was signed with unknown key 0x%1.")
                   .arg(pgp->signedByKey());
        htmlStr += "<br />";
        htmlStr += i18n("The validity of the signature can't be verified.");
        htmlStr += "\n</td></tr>\n<tr class=\"" + signClass + "B\"><td>\n";
      }
      else
      {
        goodSignature = pgp->goodSignature();

        /* HTMLize signedBy data ### FIXME: use .arg()*/
        signer.replace(QRegExp("&"), "&amp;");
        signer.replace(QRegExp("<"), "&lt;");
        signer.replace(QRegExp(">"), "&gt;");
        signer = "<a href=\"mailto:" + signer + "\">" + signer + "</a>";

        QCString keyId = pgp->signedByKey();
        Kpgp::Validity keyTrust;
        if( !keyId.isEmpty() )
          keyTrust = pgp->keyTrust( keyId );
        else
          // This is needed for the PGP 6 support because PGP 6 doesn't
          // print the key id of the signing key if the key is known.
          keyTrust = pgp->keyTrust( pgp->signedBy() );

        if (goodSignature)
        {
          if( keyTrust < Kpgp::KPGP_VALIDITY_MARGINAL )
            signClass = "signOkKeyBad";
          else
            signClass = "signOkKeyOk";
          htmlStr += "<table cellspacing=\"1\" cellpading=\"0\" "
                     "class=\"" + signClass + "\">\n"
                     "<tr class=\"" + signClass + "H\"><td>\n";
          if( !keyId.isEmpty() )
            htmlStr += i18n("Message was signed by %1 (ID: 0x%2).").arg(signer)
                                                                 .arg(keyId);
          else
            htmlStr += i18n("Message was signed by %1.").arg(signer);
          htmlStr += "<br />";
          switch( keyTrust )
          {
          case Kpgp::KPGP_VALIDITY_UNKNOWN:
            htmlStr += i18n("The signature is valid, but the key's validity is unknown.");
            break;
          case Kpgp::KPGP_VALIDITY_MARGINAL:
            htmlStr += i18n("The signature is valid and the key is marginally trusted.");
            break;
          case Kpgp::KPGP_VALIDITY_FULL:
            htmlStr += i18n("The signature is valid and the key is fully trusted.");
            break;
          case Kpgp::KPGP_VALIDITY_ULTIMATE:
            htmlStr += i18n("The signature is valid and the key is ultimately trusted.");
            break;
          default:
            htmlStr += i18n("The signature is valid, but the key is untrusted.");
          }
          htmlStr += "\n</td></tr>\n<tr class=\"" + signClass + "B\"><td>\n";
        }
        else
        {
          signClass = "signErr";
          htmlStr += "<table cellspacing=\"1\" cellpading=\"0\" "
                     "class=\"" + signClass + "\">\n"
                     "<tr class=\"" + signClass + "H\"><td>\n";
          htmlStr += i18n("Message was signed by %1.").arg(signer);
          htmlStr += "<br />";
          htmlStr += i18n("Warning: The signature is bad.");
          htmlStr += "\n</td></tr>\n<tr class=\"" + signClass + "B\"><td>\n";
        }
      }
    }
    if (isEncrypted || isSigned)
    {
      htmlStr += quotedHTML(aCodec->toUnicode(pgp->message()));
      if (isSigned) {
        htmlStr += "</td></tr>\n<tr class=\"" + signClass + "H\"><td>" +
                   i18n("End of signed message") +
                   "</td></tr></table>";
      }
      if (isEncrypted) {
        htmlStr += "</td></tr>\n<tr class=\"encrH\"><td>" +
                   i18n("End of encrypted message") +
                   "</td></tr></table>";
      }
      str = pgp->backmatter();
      if(!str.isEmpty()) htmlStr += quotedHTML(aCodec->toUnicode(str));
    } // if (!pgpMessage) then the message only looked similar to a pgp message
    else htmlStr = quotedHTML(aCodec->toUnicode(aStr));
  }
  else htmlStr = quotedHTML(aCodec->toUnicode(aStr));
  if (isEncrypted || isSigned)
  {
    mColorBar->setEraseColor( cCBpgp );
    mColorBar->setText(i18n("\nP\nG\nP\n\nM\ne\ns\ns\na\ng\ne"));
  }
  else
  {
    mColorBar->setEraseColor( cCBplain );
    mColorBar->setText(i18n("\nN\no\n\nP\nG\nP\n\nM\ne\ns\ns\na\ng\ne"));
  }
  mViewer->write(htmlStr);
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeHTMLStr(const QString& aStr)
{
  mColorBar->setEraseColor( cCBhtml );
  mColorBar->setText(i18n("\nH\nT\nM\nL\n\nM\ne\ns\ns\na\ng\ne"));
  mViewer->write(aStr);
}

//-----------------------------------------------------------------------------

QString KMReaderWin::quotedHTML(const QString& s)
{
  QString htmlStr, line, normalStartTag, normalEndTag;
  QString quoteEnd("</span>");

  unsigned int pos, beg;
  unsigned int length = s.length();

  if (mBodyFont.bold()) { normalStartTag += "<b>"; normalEndTag += "</b>"; }
  if (mBodyFont.italic()) { normalStartTag += "<i>"; normalEndTag += "</i>"; }

  // skip leading empty lines
  for( pos = 0; pos < length && s[pos] <= ' '; pos++ );
  while (pos > 0 && (s[pos-1] == ' ' || s[pos-1] == '\t')) pos--;
  beg = pos;

  htmlStr = normalStartTag;

  int currQuoteLevel = -1;
  
  while (beg<length)
  {
    /* search next occurance of '\n' */
    pos = s.find('\n', beg, FALSE);
    if (pos == (unsigned int)(-1))
	pos = length;

    line = s.mid(beg,pos-beg);
    beg = pos+1;

    /* calculate line's current quoting depth */
    unsigned int p;
    int actQuoteLevel = -1;
    bool finish = FALSE;
    for (p=0; p<line.length() && !finish; p++) {
	switch (line[p].latin1()) {
	    /* case ':': */
	    case '>':
	    case '|':	actQuoteLevel++;
			break;
	    case ' ':
	    case '\t':	break;
	    default:	finish = TRUE;
			break;
	}
    } /* for() */

    line = strToHtml(line, TRUE);
    p = line.length();
    line.append("<br>\n");

    /* continue with current quotelevel if it didn't changed */
    if (actQuoteLevel == currQuoteLevel || p == 0) {
	htmlStr.append(line);
	continue;
    }

    /* finish last quotelevel */
    if (currQuoteLevel == -1)
	htmlStr.append( normalEndTag );
    else
	htmlStr.append( quoteEnd );

    /* start new quotelevel */
    currQuoteLevel = actQuoteLevel;
    if (actQuoteLevel == -1)
	line.prepend(normalStartTag);
    else
        line.prepend( mQuoteFontTag[currQuoteLevel%3] );

    htmlStr.append(line);
  } /* while() */

  /* really finish the last quotelevel */
  if (currQuoteLevel == -1)
     htmlStr.append( normalEndTag );
  else
     htmlStr.append( quoteEnd );	

  return htmlStr;
}



//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum,
  bool quiet)
{
  QString iconName, href, label, comment, contDisp;
  QString fileName;

  if(aMsgPart == NULL) {
    kdDebug(5006) << "writePartIcon: aMsgPart == NULL\n" << endl;
    return;
  }

  kdDebug(5006) << "writePartIcon: PartNum: " << aPartNum << endl;

  comment = aMsgPart->contentDescription();
  comment.replace(QRegExp("&"), "&amp;");
  comment.replace(QRegExp("<"), "&lt;");
  comment.replace(QRegExp(">"), "&gt;");

  fileName = aMsgPart->fileName();
  if (fileName.isEmpty()) fileName = aMsgPart->name();
  label = fileName;
      /* HTMLize label */
  label.replace(QRegExp("&"), "&amp;");
  label.replace(QRegExp("<"), "&lt;");
  label.replace(QRegExp(">"), "&gt;");

//--- Sven's save attachments to /tmp start ---
  KTempFile *tempFile = new KTempFile(QString::null,
    "." + QString::number(aPartNum));
  tempFile->setAutoDelete(true);
  QString fname = tempFile->name();
  delete tempFile;

  bool ok = true;

  if (access(QFile::encodeName(fname), W_OK) != 0) // Not there or not writable
    if (mkdir(QFile::encodeName(fname), 0) != 0
      || chmod (QFile::encodeName(fname), S_IRWXU) != 0)
        ok = false; //failed create

  if (ok)
  {
    mTempDirs.append(fname);
    fileName.replace(QRegExp("[/\"\']"),"");
    if (fileName.isEmpty()) fileName = "unnamed";
    fname += "/" + fileName;

    if (!kByteArrayToFile(aMsgPart->bodyDecodedBinary(), fname, false, false, false))
      ok = false;
    mTempFiles.append(fname);
  }
  if (ok)
  {
    href = QString("file:")+fname;
    //debug ("Wrote attachment to %s", href.data());
  }
  else {
    //--- Sven's save attachments to /tmp end ---
    href = QString("part://%1").arg(aPartNum+1);
  }

  // sven: for viewing images inline
  if (inlineImage)
    iconName = href;
  else
    iconName = aMsgPart->iconName();
  if (iconName.right(14)=="mime_empty.png")
  {
    aMsgPart->magicSetType();
    iconName = aMsgPart->iconName();
  }
  if (!quiet)
    mViewer->write("<table><tr><td><a href=\"" + href + "\"><img src=\"" +
                   iconName + "\" border=\"0\">" + label +
                   "</a></td></tr></table>" + comment + "<br>");
}


//-----------------------------------------------------------------------------
QString KMReaderWin::strToHtml(const QString &aStr, bool aPreserveBlanks) const
{
  QString iStr, str;
  QString result((QChar*)NULL, aStr.length() * 2);
  int pos;
  QChar ch;
  int i, i1, x, len;
  bool startOfLine = true;

  for (pos = 0, x = 0; pos < (int)aStr.length(); pos++, x++)
  {
    ch = aStr[pos];
    if (aPreserveBlanks)
    {
      if (ch==' ')
      {
	if (startOfLine) {
	  result += "&nbsp;";
	  pos++, x++;
	  startOfLine = false;
	}
        while (aStr[pos] == ' ')
        {
	  result += " ";
          pos++, x++;
	  if (aStr[pos] == ' ') {
	    result += "&nbsp;";
	    pos++, x++;
	  }
        }
        pos--, x--;

        continue;
      }
      else if (ch=='\t')
      {
	do
	{
	  result += "&nbsp;";
	  x++;
	}
	while((x&7) != 0);
        x--;
        continue;
      }
    }
    if (ch=='&') result += "&amp;";
    else if (ch=='<') result += "&lt;";
    else if (ch=='>') result += "&gt;";
    else if (ch=='\n') {
      result += "<br>";
      startOfLine = true;
      x = -1;
    }
    else if (ch=='&') result += "&amp;";
    else if ((ch=='h' && aStr.mid(pos, 7) == "http://") ||
	     (ch=='h' && aStr.mid(pos, 8) == "https://") ||
	     (ch=='f' && aStr.mid(pos, 6) == "ftp://") ||
	     (ch=='m' && aStr.mid(pos, 7) == "mailto:"
              && (int)aStr.length() > pos+7 && aStr[pos+7] != ' '))
	     // note: no "file:" for security reasons
    {
      // handle cases like this: <link>http://foobar.org/</link>
      for (i=0; pos < (int)aStr.length() && aStr[pos] > ' '
           && aStr[pos] != '\"'
           && QString("<>()[]").find(aStr[pos]) == -1; i++, pos++)
        str[i] = aStr[pos];
      pos--;
      while (i>0 && str[i-1].isPunct() && str[i-1]!='/' && str[i-1]!='#')
      {
	i--;
	pos--;
      }
      str.truncate(i);
      result += "<a href=\"" + str + "\">" + str + "</a>";
    }
    else if (ch=='@')
    {
      int startpos = pos;
      for (i=0; pos >= 0 && aStr[pos].unicode() < 128
	     && (aStr[pos].isLetterOrNumber()
                 || QString("@._-*[]=+").find(aStr[pos]) != -1)
	     && i<255; i++, pos--)
	{
	}
      i1 = i;
      pos++;
      for (i=0; pos < (int)aStr.length() && aStr[pos].unicode() < 128
             && (aStr[pos].isLetterOrNumber()
                 || QString("@._-*[]=+").find(aStr[pos]) != -1)
	     && i<255; i++, pos++)
      {
	iStr += aStr[pos];
      }
      pos--;
      len = iStr.length();
      while (len>2 && QString("._-*()=+").find(aStr[pos]) != -1
        && (pos > startpos))
      {
	len--;
	pos--;
      }
      iStr.truncate(len);
      result.truncate(result.length() - i1 + 1);

      if (iStr.length() >= 3 && iStr.contains("@") == 1 && iStr[0] != '@'
        && iStr[iStr.length() - 1] != '@')
      {
        iStr = "<a href=\"mailto:" + iStr + "\">" + iStr + "</a>";
      }
      result += iStr;
      iStr = "";
    }

    else {
      result += ch;
      startOfLine = false;
    }
  }

  return result;
}


//-----------------------------------------------------------------------------
void KMReaderWin::printMsg(void)
{
  if (!mMsg) return;

  if (mPrinting)
    mViewer->view()->print();
  else {
    KMReaderWin printWin;
    printWin.setPrinting(TRUE);
    printWin.readConfig();
    printWin.setMsg(mMsg, TRUE);
    printWin.printMsg();
  }
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const KURL &aUrl)
{
  if (aUrl.isEmpty() || !mMsg) return -1;

  if (!aUrl.isLocalFile()) return -1;

  QString path = aUrl.path();
  uint right = path.findRev('/');
  uint left = path.findRev('.', right);

  bool ok;
  int res = path.mid(left + 1, right - left - 1).toInt(&ok);
  return (ok) ? res : -1;
}


//-----------------------------------------------------------------------------
void KMReaderWin::resizeEvent(QResizeEvent *)
{
  if( !mResizeTimer.isActive() )
  {
    //
    // Combine all resize operations that are requested as long a
    // the timer runs.
    //
    mResizeTimer.start( 100, true );
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDelayedResize()
{
  //mViewer->widget()->setGeometry(0, 0, width(), height());
  mBox->setGeometry(0, 0, width(), height());
}


//-----------------------------------------------------------------------------
void KMReaderWin::closeEvent(QCloseEvent *e)
{
  KMReaderWinInherited::closeEvent(e);
  writeConfig();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOn(const QString &aUrl)
{
  KURL url(aUrl);

  int id = msgPartFromUrl(url);
  if (id < 0)
  {
    emit statusMsg(aUrl);
  }
  else
  {
    KMMessagePart msgPart;
    mMsg->bodyPart(id, &msgPart);
    QString str = msgPart.fileName();
    if (str.isEmpty()) str = msgPart.name();
    emit statusMsg(i18n("Attachment: ") + str);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KURL &aUrl, const KParts::URLArgs &)
{
  if (!aUrl.hasHost() && aUrl.path() == "/" && aUrl.hasRef())
  {
    if (!mViewer->gotoAnchor(aUrl.ref()))
      static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);
    return;
  }
  int id = msgPartFromUrl(aUrl);
  if (id >= 0)
  {
    // clicked onto an attachment
    mAtmCurrent = id;
    mAtmCurrentName = aUrl.path();
    slotAtmOpen();
  }
  else {
//      if (aUrl.protocol().isEmpty() || (aUrl.protocol() == "file"))
//	  return;
      emit urlClicked(aUrl,/* aButton*/LeftButton); //### FIXME: add button to URLArgs!
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  if (!mMsg) return;
  KURL url( aUrl );

  int id = msgPartFromUrl(url);
  if (id < 0)
  {
    emit popupMenu(*mMsg, url, aPos);
  }
  else
  {
    // Attachment popup
    mAtmCurrent = id;
    mAtmCurrentName = url.path();
    KPopupMenu *menu = new KPopupMenu();
    menu->insertItem(i18n("Open..."), this, SLOT(slotAtmOpen()));
    menu->insertItem(i18n("Open with..."), this, SLOT(slotAtmOpenWith()));
    menu->insertItem(i18n("View..."), this, SLOT(slotAtmView()));
    menu->insertItem(i18n("Save as..."), this, SLOT(slotAtmSave()));
    menu->insertItem(i18n("Properties..."), this,
		     SLOT(slotAtmProperties()));
    menu->exec(aPos,0);
    delete menu;
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotFind()
{
  //dnaber:
  KAction *act = mViewer->actionCollection()->action("find");
  if( act )
    act->activate();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotToggleFixedFont()
{
  mUseFixedFont = !mUseFixedFont;
  mBodyFamily = (mUseFixedFont) ? mFixedFont.family() : mBodyFont.family();
  fntSize = (mUseFixedFont) ? mFixedFont.pointSize() : mBodyFont.pointSize();
  update(true);  
}

//-----------------------------------------------------------------------------
void KMReaderWin::atmViewMsg(KMMessagePart* aMsgPart)
{
  KMMessage* msg = new KMMessage;
  assert(aMsgPart!=NULL);

  msg->fromString(aMsgPart->bodyDecoded());
  emit showAtmMsg(msg);
}


//-----------------------------------------------------------------------------
void KMReaderWin::atmView(KMReaderWin* aReaderWin, KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname, QTextCodec *codec)
{
  QString str;

  if (aReaderWin && qstricmp(aMsgPart->typeStr(), "message")==0)
  {
    aReaderWin->atmViewMsg(aMsgPart);
    return;
  }

  kernel->kbp()->busy();
  {
    KMReaderWin* win = new KMReaderWin; //new reader
    if (qstricmp(aMsgPart->typeStr(), "message")==0)
    {               // if called from compose win
      KMMessage* msg = new KMMessage;
      assert(aMsgPart!=NULL);
      msg->fromString(aMsgPart->bodyDecoded());
      win->setCaption(msg->subject());
      win->setMsg(msg, true);
      win->show();
    }
    else if (qstricmp(aMsgPart->typeStr(), "text")==0)
    {
      if (qstricmp(aMsgPart->subtypeStr(), "x-vcard") == 0) {
        KMDisplayVCard *vcdlg;
	int vcerr;
	VCard *vc = VCard::parseVCard(codec->toUnicode(aMsgPart
          ->bodyDecoded()), &vcerr);

	if (!vc) {
          QString errstring = i18n("Error reading in vCard:\n");
	  errstring += VCard::getError(vcerr);
	  KMessageBox::error(NULL, errstring, i18n("vCard error"));
	  return;
	}

	vcdlg = new KMDisplayVCard(vc);
        kernel->kbp()->idle();
	vcdlg->show();
	return;
      }
      win->readConfig();
      if ( codec )
	win->setCodec( codec );
      else
	win->setCodec( KGlobal::charsets()->codecForName( "iso8859-1" ) );
      win->mViewer->begin( KURL( "file:/" ) );
      win->mViewer->write("<html><head><style type=\"text/css\">" +
		 QString("a { color: #%1;").arg(win->colorToString(win->c2)) +
		 "text-decoration: none; }" + // just playing
		 "</style></head><body " +
                 QString(" text=\"#%1\"").arg(win->colorToString(win->c1)) +
  		 QString(" bgcolor=\"#%1\"").arg(win->colorToString(win->c4)) +
		 ">" );

      QCString str = aMsgPart->bodyDecoded();
      if (aHTML && (qstricmp(aMsgPart->subtypeStr(), "html")==0))  // HTML
        //win->mViewer->write(win->codec()->toUnicode(str));
	win->writeHTMLStr(win->codec()->toUnicode(str));
      else  // plain text
	win->writeBodyStr(str, win->codec());
      win->mViewer->write("</body></html>");
      win->mViewer->end();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    }
    else if (qstricmp(aMsgPart->typeStr(), "image")==0)
    {
      if (aFileName.isEmpty()) return;  // prevent crash
      // Open the window with a size so the image fits in (if possible):
      QImageIO *iio = new QImageIO();
      iio->setFileName(aFileName);
      if( iio->read() ) {
        QImage img = iio->image();
	if( img.width() > 50 && img.width() > 50	// avoid super small windows
	    && img.width() < KApplication::desktop()->width()	// avoid super large windows
	    && img.height() < KApplication::desktop()->height() ) {
	  win->resize(img.width()+10, img.height()+10);
	}
      }
      // Just write the img tag to HTML:
      win->mViewer->begin( KURL( "file:/" ) );
      win->mViewer->write("<html><body>");
      QString linkName = QString("<img src=\"file:%1\" border=0>").arg(aFileName);
      win->mViewer->write(linkName);
      win->mViewer->write("</body></html>");
      win->mViewer->end();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    } else {
      QMultiLineEdit *medit = new QMultiLineEdit();
      QString str = aMsgPart->bodyDecoded();
      // A QString cannot handle binary data. So if it's shorter than the
      // attachment, we assume the attachment is binary:
      if( str.length() < (unsigned) aMsgPart->size() ) {
        str += i18n("\n[KMail: Attachment contains binary data. Trying to show first %1 characters.]").arg(str.length());
      }
      medit->setText(str);
      medit->setReadOnly(true);
      medit->resize(500, 550);
      medit->show();
    }
  }
  // ---Sven's view text, html and image attachments in html widget end ---
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmView()
{
  KMMessagePart msgPart;
  mMsg->bodyPart(mAtmCurrent, &msgPart);
  QString pname = msgPart.fileName();
  if (pname.isEmpty()) pname=msgPart.name();
  if (pname.isEmpty()) pname=msgPart.contentDescription();
  if (pname.isEmpty()) pname="unnamed";
  // image Attachment is saved already
  QTextCodec *atmCodec = (mAutoDetectEncoding) ?
    KMMsgBase::codecForName(msgPart.charset()) : mCodec;
  if (!atmCodec) atmCodec = mCodec;
  atmView(this, &msgPart, htmlMail(), mAtmCurrentName, pname, atmCodec);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  QString str, pname, cmd, fileName;
  KMMessagePart msgPart;

  mMsg->bodyPart(mAtmCurrent, &msgPart);
  if (qstricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  if (qstricmp(msgPart.typeStr(), "text") == 0)
  {
    if (qstricmp(msgPart.subtypeStr(), "x-vcard") == 0) {
      KMDisplayVCard *vcdlg;
      int vcerr;
      QTextCodec *atmCodec = (mAutoDetectEncoding) ?
        KMMsgBase::codecForName(msgPart.charset()) : mCodec;
      if (!atmCodec) atmCodec = mCodec;
      VCard *vc = VCard::parseVCard(atmCodec->toUnicode(msgPart
        .bodyDecoded()), &vcerr);

      if (!vc) {
        QString errstring = i18n("Error reading in vCard:\n");
        errstring += VCard::getError(vcerr);
        KMessageBox::error(this, errstring, i18n("vCard error"));
        return;
      }

      vcdlg = new KMDisplayVCard(vc);
      vcdlg->show();
      return;
    }
  }

  // What to do when user clicks on an attachment --dnaber, 2000-06-01
  // TODO: show full path for Service, not only name
  QString mimetype = KMimeType::findByURL(KURL(mAtmCurrentName))->name();
  KService::Ptr offer = KServiceTypeProfile::preferredService(mimetype, "Application");
  QString question;
  QString open_text = i18n("Open");
  QString filenameText = msgPart.fileName();
  if (filenameText.isEmpty()) filenameText = msgPart.name();
  if ( offer ) {
    question = i18n("Open attachment '%1' using '%2'?").arg(filenameText)
      .arg(offer->name());
  } else {
    question = i18n("Open attachment '%1'?").arg(filenameText);
    open_text = i18n("Open With...");
  }
  question += i18n("\n\nNote that opening an attachment may compromise your system's security!");
  // TODO: buttons don't have the correct order, but "Save" should be default
  int choice = KMessageBox::warningYesNoCancel(this, question,
      i18n("Open Attachment?"), i18n("Save to Disk"), open_text);
  if( choice == KMessageBox::Yes ) {		// Save
    slotAtmSave();
  } else if( choice == KMessageBox::No ) {	// Open
    if ( offer ) {
      // There's a default service for this kind of file - use it
      KURL::List lst;
      KURL url;
      url.setPath(mAtmCurrentName);
      lst.append(url);
      KRun::run(*offer, lst);
    } else {
      // There's no know service that handles this type of file, so open
      // the "Open with..." dialog.
      KFileOpenWithHandler *openhandler = new KFileOpenWithHandler();
      KURL::List lst;
      KURL url;
      url.setPath(mAtmCurrentName);
      lst.append(url);
      openhandler->displayOpenWithDialog(lst);
    }
  } else {					// Cancel
    kdDebug(5006) << "Canceled opening attachment" << endl;
  }

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpenWith()
{
  // It makes sense to have an extra "Open with..." entry in the menu
  // so the user can change filetype associations.

  KMMessagePart msgPart;

  mMsg->bodyPart(mAtmCurrent, &msgPart);

  KFileOpenWithHandler *openhandler = new KFileOpenWithHandler();
  KURL::List lst;
  KURL url;
  url.setPath(mAtmCurrentName);
  lst.append(url);
  openhandler->displayOpenWithDialog(lst);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  KMMessagePart msgPart;
  QString fileName;
  fileName = mSaveAttachDir;

  mMsg->bodyPart(mAtmCurrent, &msgPart);
  if (!msgPart.fileName().isEmpty())
    fileName.append(msgPart.fileName());
  else
    fileName.append(msgPart.name());

  KURL url = KFileDialog::getSaveURL( fileName, QString::null, this );

  if( url.isEmpty() )
    return;

  mSaveAttachDir = url.directory() + "/";

  kernel->byteArrayToRemoteFile(msgPart.bodyDecodedBinary(), url);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmProperties()
{
  KMMessagePart msgPart;
  KMMsgPartDlg  dlg(0,TRUE);

  kernel->kbp()->busy();
  mMsg->bodyPart(mAtmCurrent, &msgPart);
  dlg.setMsgPart(&msgPart);
  kernel->kbp()->idle();

  dlg.exec();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollUp()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, -10);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, 10);
}

bool KMReaderWin::atBottom() const
{
    const QScrollView *view = static_cast<const QScrollView *>(mViewer->widget());
    return view->contentsY() + view->visibleHeight() >= view->contentsHeight();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotJumpDown()
{
    QScrollView *view = static_cast<QScrollView *>(mViewer->widget());
    int offs = (view->clipper()->height() < 30) ? view->clipper()->height() : 30;
    view->scrollBy( 0, view->clipper()->height() - offs );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollPrior()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, -(int)(height()*0.8));
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollNext()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, (int)(height()*0.8));
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentChanged()
{

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotTextSelected(bool)
{

  QString temp = mViewer->selectedText();
  kapp->clipboard()->setText(temp);
}

//-----------------------------------------------------------------------------
void KMReaderWin::selectAll()
{
  mViewer->selectAll();
}

//-----------------------------------------------------------------------------
QString KMReaderWin::copyText()
{
  QString temp = mViewer->selectedText();
  return temp;
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentDone()
{
  // mSbVert->setValue(0);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlOverride(bool override)
{
  mHtmlOverride = override;
  if (mMsg) mMsg->setDecodeHTML(htmlMail());
}


//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}

//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"
