// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <qstring.h>
#include <qdir.h>
#include <qbitmap.h>
#include <qclipboard.h>
#include <qcursor.h>
#include <qmultilineedit.h>
#include <qregexp.h>
#include <qscrollbar.h>
#include <qimage.h>
#include <kaction.h>

#include <kfiledialog.h>
#include <khtml_part.h>
#include <khtmlview.h> // So that we can get rid of the frames
#include <kapp.h>
#include <kconfig.h>
#include <kcharsets.h>
#include <kcursor.h>
#include <krun.h>
#include <kpopupmenu.h>
#include <kopenwith.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <ktempfile.h>

#include <mimelib/mimepp.h>

#include "kmversion.h"
#include "kmglobal.h"
#include "kmmainwin.h"

#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmreaderwin.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include <kpgp.h>
#include "kurl.h"

// for selection
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

//--- Sven's save attachments to /tmp start ---
#include <unistd.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kstddirs.h>  // for access and getpid
//--- Sven's save attachments to /tmp end ---

// for the click on attachment stuff (dnaber):
#include <kmessagebox.h>
#include <kuserprofile.h>
#include <kurl.h>
#include <kmimemagic.h>
#include <kmimetype.h>

// Do the tmp stuff correctly - thanks to Harri Porten for
// reminding me (sven)

#include "vcard.h"
#include "kmdisplayvcard.h"

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

const int KMReaderWin::delay = 150;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent, const char *aName, int aFlags)
  :KMReaderWinInherited(aParent, aName, aFlags | Qt::WDestructiveClose)
{

  mAutoDelete = FALSE;
  mMsg = 0;
  mMsgBuf = 0;
  mMsgBufMD5 = "";
  mMsgBufSize = -1;
  mMsgDisplay = TRUE;

  initHtmlWidget();
  readConfig();
  mHtmlOverride = false;

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

  c1 = QColor(kapp->palette().normal().text());
  c2 = KGlobalSettings::linkColor();
  c3 = KGlobalSettings::visitedLinkColor();
  c4 = QColor(kapp->palette().normal().base());

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    c1 = config->readColorEntry("ForegroundColor",&c1);
    c2 = config->readColorEntry("LinkColor",&c2);
    c3 = config->readColorEntry("FollowedColor",&c3);
    c4 = config->readColorEntry("BackgroundColor",&c4);
  }
  else {
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
  }

  {
  KConfigGroupSaver saver(config, "Fonts");
  mUnicodeFont = config->readBoolEntry("unicodeFont",FALSE);
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    mBodyFont = QFont("helvetica");
    mBodyFont = config->readFontEntry("body-font", &mBodyFont);
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
      color = QColor(kapp->palette().normal().text());
    }
    else
    {
      QColor defaultColor = QColor(kapp->palette().normal().text());
      if( quoteLevel == 0 )
	color = config->readColorEntry( "QuoutedText1", &defaultColor );
      else if( quoteLevel == 1 )
	color = config->readColorEntry( "QuoutedText2", &defaultColor );
      else if( quoteLevel == 2 )
	color = config->readColorEntry( "QuoutedText3", &defaultColor );
      else
	color = QColor(kapp->palette().normal().base());
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

  QString str = QString("<font color=\"%1\">").arg( color.name() );
  if( font.italic() ) { str += "<i>"; }
  if( font.bold() ) { str += "<b>"; }
  return( str );
}


//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  mViewer = new KHTMLPart(this, "khtml");
  mViewer->widget()->setFocusPolicy(WheelFocus);
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->setPluginsEnabled(false);
  mViewer->setJScriptEnabled(false); // just make this explicit
  mViewer->setJavaEnabled(false);    // just make this explicit
  mViewer->setMetaRefreshEnabled(false);
  mViewer->widget()->resize(width()-16, height()-110);
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
void KMReaderWin::setBodyFont(const QFont aFont)
{
  mBodyFont = aFont;
  update(true);
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
  removeTempFiles();
  if (aMsg)
      kdDebug(5006) << aMsg->subject() << " " << aMsg->fromStrip() << endl;

  // If not forced and there is aMsg and aMsg is same as mMsg then return
  //if (!force && aMsg && mMsg == aMsg)
  //  return;

  kdDebug(5006) << "Not equal" << endl;

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


//-----------------------------------------------------------------------------
void KMReaderWin::displayAboutPage()
{
  mMsgDisplay = FALSE;
  QString location = locate("data", "kmail/about/main.html");
  QString content = kFileToString(location);
  mViewer->closeURL();
  mViewer->begin(location);
  QString info =
    i18n("<h2>Welcome to KMail %1</h2><p>KMail is an email client for the K "
    "Desktop Environment. It is designed to be fully compatible with Internet "
    "mailing standards including MIME, SMTP, POP3 and IMAP.</p>\n"
    "<ul><li>KMail has many powerful features which are described in the "
    "<A HREF=\"%2\">documentation</A></li>\n"
    "<li>The <A HREF=\"%3\">KMail homepage</A> offers information about "
    "new versions of KMail</li></ul>\n").arg(KMAIL_VERSION)
    .arg("help:/kmail")
    .arg("http://kmail.kde.org/") +
    i18n("<p>Some of the new features in this release of KMail include "
    "(compared to KMail 1.3, which is part of KDE 2.2):</p>\n"
    "<ul>\n"
    "<li>Maildir support</li>\n"
    "<li>SMTP authentication</li>\n"
    "<li>SMTP over SSL/TLS</li>\n"
    "<li>Pipelining for POP3 (faster mail download on slow responding networks)</li>\n"
    "<li>Performance improvements for IMAP</li>\n"
    "<li>Automatic configuration of the POP3/IMAP/SMTP security features</li>\n"
    "<li>DIGEST-MD5 authentication</li>\n"
    "</ul>\n");
  if( kernel->firstStart() ) {
    info += i18n("<p>Please take a moment to fill in the KMail configuration panel at "
    "Settings-&gt;Configuration.\n"
    "You need to at least create a primary identity and a mail "
    "account.</p>\n");
  }
  info += i18n("<p>We hope that you will enjoy KMail.</p>\n"
    "<p>Thank you,</p>\n"
    "<p>&nbsp; &nbsp; The KMail Team</p>");
  mViewer->write(content.arg(fntSize).arg(info));
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
  if (mMsgBuf && mMsg &&
      !mMsg->msgIdMD5().isEmpty() &&
      (mMsgBufMD5 == mMsg->msgIdMD5()) &&
      ((unsigned)mMsgBufSize == mMsg->msgSize()))
    return;

  mMsgBuf = mMsg;
  if (mMsgBuf) {
    mMsgBufMD5 = mMsgBuf->msgIdMD5();
    mMsgBufSize = mMsgBuf->msgSize();
  }
  else {
    mMsgBufMD5 = "";
    mMsgBufSize = -1;
  }

  if (mMsg && !mMsg->msgIdMD5().isEmpty())
    updateReaderWinTimer.start( delay, TRUE );

  if (!mMsgDisplay) return;

  mViewer->view()->setUpdatesEnabled( false );
  mViewer->view()->viewport()->setUpdatesEnabled( false );
  static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);
  if (mMsg) parseMsg();
  else
  {
    mViewer->closeURL();
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

  mViewer->closeURL();
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
      encoding = "iso8859-1";
    mCodec = KMMsgBase::codecForName(encoding);
  }

  if (!mCodec)
    mCodec = QTextCodec::codecForName("iso8859-1");
  mMsg->setCodec(mCodec);

  mViewer->write("<html><head><style type=\"text/css\">" +
		 QString("body { font-family: \"%1\"; font-size: %2pt }\n")
                 .arg( mBodyFamily ).arg( fntSize ) +
		 QString("a { color: #%1; ").arg(colorToString(c2)) +
		 "text-decoration: none; }" + // just playing
		 "</style></head><body " +
		 // TODO: move these to stylesheet, too:
                 QString(" text=\"#%1\"").arg(colorToString(c1)) +
  		 QString(" bgcolor=\"#%1\"").arg(colorToString(c4)) +
                 bkgrdStr + ">" );

  if (!parent())
    setCaption(mMsg->subject());

  parseMsg(mMsg);

  mViewer->write("</body></html>");
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
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
  int vcnum = -1;
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
          vcnum = j;
          writePartIcon(&msgPart, j, TRUE);
          break;
        }
    }
  }

  writeMsgHeader(vcnum);

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
          mViewer->write(mCodec->toUnicode(str.data()));    // write it...
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
            mViewer->write(atmCodec->toUnicode(cstr.data()));
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
      mViewer->write(mCodec->toUnicode(aMsg->bodyDecoded().data()));
    else
      writeBodyStr(aMsg->bodyDecoded(), mCodec);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeMsgHeader(int vcpartnum)
{
  QString str;
  QString vcname;

  if (vcpartnum >= 0) vcname = mTempFiles.last();

  switch (mHeaderStyle)
  {
    case HdrBrief:
    mViewer->write("<b style=\"font-size:130%\">" + strToHtml(mMsg->subject()) +
                   "</b>&nbsp; (" +
                   KMMessage::emailAddrAsAnchor(mMsg->from(),TRUE) + ", ");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->cc(),TRUE) + ", ");
    mViewer->write("&nbsp;"+strToHtml(mMsg->dateShortStr()) + ")");
    if (vcpartnum >= 0) {
      mViewer->write("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    mViewer->write("<br>\n");
    break;

  case HdrStandard:
    mViewer->write("<b style=\"font-size:130%\">" +
                   strToHtml(mMsg->subject()) + "</b><br>\n");
    mViewer->write(i18n("From: ") +
                   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (vcpartnum >= 0) {
      mViewer->write("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    mViewer->write("<br>\n");
    mViewer->write(i18n("To: ") +
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>\n");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>\n");
    break;

  case HdrFancy:
    mViewer->write(QString("<table><tr><td><img src=") +
		   locate("data", "kmail/pics/kdelogo.xpm") +
                   "></td><td hspace=\"50\"><b style=\"font-size:160%\">");
    mViewer->write(strToHtml(mMsg->subject()) + "</b><br>");
    mViewer->write(i18n("From: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (vcpartnum >= 0) {
      mViewer->write("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    mViewer->write("<br>\n");
    mViewer->write(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>\n");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>\n");
    mViewer->write(i18n("Date: ")+
                   strToHtml(mMsg->dateStr()) + "<br>\n");
    mViewer->write("</b></td></tr></table>");
    break;

  case HdrLong:
    mViewer->write("<b style=\"font-size:130%\">" +
                   strToHtml(mMsg->subject()) + "</b><br>");
    mViewer->write(i18n("Date: ")+strToHtml(mMsg->dateStr())+"<br>");
    mViewer->write(i18n("From: ")+
		   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (vcpartnum >= 0) {
      mViewer->write("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    mViewer->write("<br>\n");
    mViewer->write(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>");
    if (!mMsg->bcc().isEmpty())
      mViewer->write(i18n("Bcc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->bcc(),FALSE) + "<br>");
    if (!mMsg->replyTo().isEmpty())
      mViewer->write(i18n("Reply to: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->replyTo(),FALSE) + "<br>");
    break;

  case HdrAll:
    str = strToHtml(mMsg->headerAsString(), true);
    mViewer->write(str);
    mViewer->write("\n");
    if (vcpartnum >= 0) {
      mViewer->write("\n<br>\n<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    break;

  default:
    kdDebug(5006) << "Unsupported header style " << mHeaderStyle << endl;
  }
  mViewer->write("<br>\n");
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr(const QCString aStr, QTextCodec *aCodec)
{
  QString line, sig, htmlStr;
  Kpgp* pgp = Kpgp::getKpgp();
  assert(pgp != NULL);
  // assert(!aStr.isNull());
  bool pgpMessage = false;

  if (pgp->setMessage(aStr))
  {
    QCString str = pgp->frontmatter();
    if(!str.isEmpty()) htmlStr += quotedHTML(aCodec->toUnicode(str));
    htmlStr += "<br>";
    if (pgp->isEncrypted())
    {
      emit noDrag();
      pgpMessage = true;
      if(pgp->decrypt())
      {
	htmlStr += QString("<b>%1</b><br>").arg(i18n("Encrypted message"));
      }
      else
      {
	htmlStr += QString("<b>%1</b><br>%2<br><br>")
                    .arg(i18n("Cannot decrypt message:"))
                    .arg(pgp->lastErrorMsg());
      }
    }
    // check for PGP signing
    if (pgp->isSigned())
    {
      pgpMessage = true;
      if (pgp->goodSignature())
         sig = i18n("Message was signed by");
      else
	 sig = i18n("Warning: Bad signature from");

      /* HTMLize signedBy data ### FIXME: use .arg()*/
      QString sdata=pgp->signedBy();
      sdata.replace(QRegExp("&"), "&amp;");
      sdata.replace(QRegExp("<"), "&lt;");
      sdata.replace(QRegExp(">"), "&gt;");

      if (sdata.contains(QRegExp("unknown key ID")))
      {
         sdata.replace(QRegExp("unknown key ID"), i18n("unknown key ID"));
         htmlStr += QString("<b>%1 %2</b><br>").arg(sig).arg(sdata);
      }
      else {
         htmlStr += QString("<b>%1 <a href=\"mailto:%2\">%3</a></b><br>")
                      .arg(sig).arg(sdata).arg(sdata);
      }
    }
    if (pgpMessage)
    {
      htmlStr += quotedHTML(aCodec->toUnicode(pgp->message()));
      htmlStr += QString("<br><b>%1</b><br><br>")
        .arg(i18n("End of pgp message"));
      str = pgp->backmatter();
      if(!str.isEmpty()) htmlStr += quotedHTML(aCodec->toUnicode(str));
    } // if (!pgpMessage) then the message only looked similar to a pgp message
    else htmlStr = quotedHTML(aCodec->toUnicode(aStr));
  }
  else htmlStr = quotedHTML(aCodec->toUnicode(aStr));
  mViewer->write(htmlStr);
}




//-----------------------------------------------------------------------------

QString KMReaderWin::quotedHTML(const QString& s)
{
  QString htmlStr, line, tmpStr, normalStartTag, normalEndTag;
  QChar ch;

  bool atStartOfLine;
  int pos, beg;

  int currQuoteLevel = -1;
  int prevQuoteLevel = -1;
  int newlineCount = 0;
  if (mBodyFont.bold()) { normalStartTag += "<b>"; normalEndTag += "</b>"; }
  if (mBodyFont.italic()) { normalStartTag += "<i>"; normalEndTag += "</i>"; }
  tmpStr = "<div>" + normalStartTag; //work around KHTML slowness

  // skip leading empty lines
  for( pos = 0; pos < (int)s.length() && s[pos] <= ' '; pos++ );
  while (pos > 0 && (s[pos-1] == ' ' || s[pos-1] == '\t')) pos--;
  beg = pos;

  atStartOfLine = TRUE;
  while( pos < (int)s.length() )
  {
    ch = s[pos];
    if(( ch == '\n' ) || ((unsigned)pos == s.length() - 1))
    {
      int adj = (ch == '\n') ? 0 : 1;
      newlineCount ++;
      line = strToHtml(s.mid(beg,pos-beg+adj),TRUE);
      if( currQuoteLevel >= 0 )
      {
	if( currQuoteLevel != prevQuoteLevel )
	{
	  line.prepend( mQuoteFontTag[currQuoteLevel%3] );
	  if( prevQuoteLevel >= 0 )
	  {
	    line.prepend( "</font>" );
	  } else line.prepend( normalEndTag );
	}
	prevQuoteLevel = currQuoteLevel;
      }
      else if( prevQuoteLevel >= 0 )
      {
        line.prepend( normalStartTag );
	line.prepend( "</font>" );
	prevQuoteLevel = -1;
      }

      tmpStr += line + "<br>";
      if( (newlineCount % 100) == 0 )
      {
	htmlStr += tmpStr;
	if (currQuoteLevel >= 0)
	  htmlStr += "</font>";
        else htmlStr += normalEndTag;
	htmlStr += "</div><div>"; //work around KHTML slowness
	if (currQuoteLevel >= 0)
	  htmlStr += mQuoteFontTag[currQuoteLevel%3];
        else htmlStr += normalStartTag;
	tmpStr.truncate(0);
      }

      beg = pos + 1;
      atStartOfLine = TRUE;
      currQuoteLevel = -1;

    }
    else if( ch > ' ' )
    {
      if( atStartOfLine == TRUE && (ch=='>' || /*ch==':' ||*/ ch=='|') )
      {
	if( mRecyleQouteColors == true || currQuoteLevel < 2 )
	{
	  currQuoteLevel += 1;
	}
      }
      else
      {
	atStartOfLine = FALSE;
      }
    }

    pos++;
  }

  if (prevQuoteLevel >= 0)
    tmpStr += "</font>";

  htmlStr += tmpStr;
  htmlStr += "</div>"; //work around KHTML slowness
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
      for (i=0; aStr[pos] && aStr[pos] > ' ' && aStr[pos] != '\"' &&
                aStr[pos] != '<' &&		// handle cases like this: <link>http://foobar.org/</link>
		i < 255; i++, pos++)
	str[i] = aStr[pos];
      pos--;
      while (i>0 && str[i].isPunct() && str[i-1]!='/')
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
                 || QString("@._-*[]=").find(aStr[pos]) != -1)
	     && i<255; i++, pos--)
	{
	}
      i1 = i;
      pos++;
      for (i=0; pos < (int)aStr.length() && aStr[pos].unicode() < 128
             && (aStr[pos].isLetterOrNumber()
                 || QString("@._-*[]=").find(aStr[pos]) != -1)
	     && i<255; i++, pos++)
      {
	iStr += aStr[pos];
      }
      pos--;
      len = iStr.length();
      while (len>2 && aStr[pos].isPunct() && (pos > startpos))
      {
	len--;
	pos--;
      }
      iStr.truncate(len);

      result.truncate(result.length() - i1 + 1);
      if (iStr.length()>3 && iStr[0] != '@'
        && iStr[iStr.length() - 1] != '@')
          iStr = "<a href=\"mailto:" + iStr + "\">" + iStr + "</a>";
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
  if (c4 == QColor(255,255,255)) { mViewer->view()->print(); return; }

  QColor hold = c4;
  c4 = QColor(255,255,255);
  update(true);
  mViewer->view()->print();
  c4 = hold;
  update(true);
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
  mViewer->widget()->setGeometry(0, 0, width(), height());
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
  if (id <= 0)
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
  if (id > 0)
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
  KURL url( aUrl );

  int id = msgPartFromUrl(url);
  if (id <= 0)
  {
    emit popupMenu(url, aPos);
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
    menu->popup(aPos,0);
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
	win->mViewer->write(win->codec()->toUnicode(str));
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
  KService::Ptr offer = KServiceTypeProfile::preferredService(mimetype, true);
  QString question;
  QString open_text = i18n("Open");
  QString filenameText = msgPart.fileName();
  if (filenameText.isEmpty()) filenameText = msgPart.name();
  if ( offer ) {
    question = i18n("Open attachment '%1' using '%2'?").arg(filenameText)
      .arg(offer->name());
  } else {
    question = i18n("Open attachment '%1'?").arg(filenameText);
    open_text = i18n("Open with...");
  }
  question += i18n("\n\nNote that opening an attachment may compromise your system's security!");
  // TODO: buttons don't have the correct order, but "Save" should be default
  int choice = KMessageBox::warningYesNoCancel(this, question,
      i18n("Open Attachment?"), i18n("Save to disk"), open_text);
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
