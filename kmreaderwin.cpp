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

#include <mimelib/mimepp.h>

#ifndef KRN
#include "kmglobal.h"
#include "kmmainwin.h"
#else
#endif

#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmreaderwin.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include "kpgp.h"
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

#ifdef KRN
extern KApplication *app;
extern KBusyPtr *kbp;
#endif

QString KMReaderWin::mAttachDir;
const int KMReaderWin::delay = 150;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent, const char *aName, int aFlags)
  :KMReaderWinInherited(aParent, aName, aFlags | Qt::WDestructiveClose)
{
  initMetaObject();

  mAutoDelete = FALSE;
  mMsg = 0;
  mMsgBuf = 0;
  mMsgBufMD5 = "";
  mMsgBufSize = -1;
  mMsgDisplay = TRUE;

  initHtmlWidget();
  readConfig();
  mHtmlOverride = false;

  if (mAttachDir.isNull())
  {
    makeAttachDir();
  }

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
}


//-----------------------------------------------------------------------------
void KMReaderWin::makeAttachDir(void)
{
  QString directory;
  directory.sprintf("kmail%d/", getpid());
  mAttachDir = locateLocal( "tmp", directory );

  if (mAttachDir.isNull()) qWarning(i18n("Failed to create temporary "
					"attachment directory '%2': %1")
					.arg(strerror(errno))
					.arg(directory));
}


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
#ifdef KRN
  config->setGroup("ArticleListOptions");
#else
  config->setGroup("Reader");
#endif
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

  config->setGroup("Pixmaps");
  mBackingPixmapOn = FALSE;
  mBackingPixmapStr = config->readEntry("Readerwin","");
  if (mBackingPixmapStr != "")
    mBackingPixmapOn = TRUE;

  config->setGroup("Reader");
  mHtmlMail = config->readBoolEntry( "htmlMail", false );
  mAtmInline = config->readNumEntry("attach-inline", 100);
  mHeaderStyle = (HeaderStyle)config->readNumEntry("hdr-style", HdrFancy);
  mAttachmentStyle = (AttachmentStyle)config->readNumEntry("attmnt-style",
							   SmartAttmnt);
  encoding = config->readEntry("encoding", "iso8859-1" );
  mCodec = KMMsgBase::codecForName(encoding);
  mAutoDetectEncoding = config->readBoolEntry("autodetect-encoding", true );

#ifndef KRN
  int i, diff;
  fntSize = 0;

  config->setGroup("Fonts");
  mUnicodeFont = config->readBoolEntry("unicodeFont",FALSE);
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    mBodyFont = QFont("helvetica");
    mBodyFont = config->readFontEntry("body-font", &mBodyFont);
    fntSize = mBodyFont.pointSize();
    mBodyFamily = mBodyFont.family();
    int pos = mBodyFamily.find("-");
    // We ignore the foundary, otherwise we can't set the charset
    if (pos != -1)
    {
      mBodyFamily.remove(0, pos + 1);
      mBodyFont.setFamily(mBodyFamily);
    }
  }
  else {
    setFont(KGlobalSettings::generalFont());
    fntSize = KGlobalSettings::generalFont().pointSize();
    mBodyFamily = KGlobalSettings::generalFont().family();
  }
  mViewer->setStandardFont(mBodyFamily);

  QValueList<int> fontsizes;
  mViewer->resetFontSizes();
  diff = fntSize - mViewer->fontSizes()[3];
  if (mViewer->fontSizes()[0]+diff > 0) {
    for (i=0;i<7; i++)
      fontsizes << mViewer->fontSizes()[i] + diff;
    mViewer->setFontSizes(fontsizes);
  }

#else
  mViewer->setDefaultFontBase(config->readNumEntry("DefaultFontBase",3));
  mViewer->setStandardFont(config->readEntry("StandardFont","helvetica"));
  mViewer->setFixedFont(config->readEntry("FixedFont","courier"));
#endif

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
  QString encoding = "";
  if (mCodec)
      encoding = mCodec->name();

  config->setGroup("Reader");
  config->writeEntry("attach-inline", mAtmInline);
  config->writeEntry("hdr-style", (int)mHeaderStyle);
  config->writeEntry("attmnt-style",(int)mAttachmentStyle);
  config->writeEntry("encoding", encoding);
  config->writeEntry("autodetect-encoding", mAutoDetectEncoding);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
QString KMReaderWin::quoteFontTag( int quoteLevel )
{
  KConfig &config = *kapp->config();

  QColor color;
  config.setGroup("Reader");
  if( config.readBoolEntry( "defaultColors", true ) == true )
  {
    color = QColor(kapp->palette().normal().text());
  }
  else
  {
    QColor defaultColor = QColor(kapp->palette().normal().text());
    if( quoteLevel == 0 )
      color = config.readColorEntry( "QuoutedText1", &defaultColor );
    else if( quoteLevel == 1 )
      color = config.readColorEntry( "QuoutedText2", &defaultColor );
    else if( quoteLevel == 2 )
      color = config.readColorEntry( "QuoutedText3", &defaultColor );
    else
      color = QColor(kapp->palette().normal().base());
  }

  QFont font;
  config.setGroup("Fonts");
  if( config.readBoolEntry( "defaultFonts", true ) == true )
  {
    font = KGlobalSettings::generalFont();
    font.setItalic(true);
  }
  else
  {
    const QFont defaultFont = QFont("helvetica");
    if( quoteLevel == 0 )
      font  = config.readFontEntry( "quote1-font", &defaultFont );
    else if( quoteLevel == 1 )
      font  = config.readFontEntry( "quote2-font", &defaultFont );
    else if( quoteLevel == 2 )
      font  = config.readFontEntry( "quote3-font", &defaultFont );
    else
    {
      font = KGlobalSettings::generalFont();
      font.setItalic(true);
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
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->enablePlugins(false);
  mViewer->enableJScript(false); // just make this explicit
  mViewer->enableJava(false);    // just make this explicit
  mViewer->enableMetaRefresh(false);
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
  if(mViewer)
    if (mUnicodeFont)
      mViewer->setCharset("iso10646-1", true);
    else mViewer->setCharset(codec->name(), true);
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
  // If not forced and there is aMsg and aMsg is same as mMsg then return
  if (!force && aMsg && mMsg == aMsg)
    return;

  mMsg = aMsg;
  if (mMsg) mMsg->setCodec(mCodec);

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
  QTextCodec *codec = QTextCodec::codecForName(KGlobal::locale()->charset());
  if (codec) mViewer->setCharset(codec->name(), true);
    else mViewer->setCharset(KGlobal::locale()->charset(), true);
  mViewer->write(content.arg(
    i18n("<h2>Welcome to KMail 1.2</h2><p>KMail is an email client for the K "
    "Desktop Environment. It is designed to be fully compatible with Internet "
    "mailing standards including MIME, SMTP and POP3.</p>\n"
    "<ul><li>KMail has many powerful features which are described in the "
    "<A HREF=\"%2\">documentation</A></li>\n"
    "<li>You can find news and updates at the <A HREF=\"%3\">KMail homepage"
    "</A></li></ul>\n"
    "<p>Some of the new features in this release of KMail include "
    "(compared to KMail 1.1.99, which is part of KDE 2.0):</p>\n"
    "<ul><li>Full support for mails in all languages and charsets supported "
    "by Qt</li>\n"
    "<li>Drafts folder</li>\n"
    "<li>Mailinglist aware folders</li>\n"
    "<li>GUI configuration of preferred PGP tool</li>\n"
    "<li>Lots of other enhancements and bugfixes</li></ul>\n"
    "<p>Please take a moment to fill in the KMail configuration panel at "
    "Settings-&gt;Configuration.\n"
    "You need to at least create a primary identity and setup a mail "
    "spool/POP3 account.</p>\n"
    "<p>We hope that you will enjoy KMail.</p>\n"
    "<p>Thank you,</p>\n"
    "<p>&nbsp; &nbsp; The KMail Team</p>")).arg("help:kmail")
    .arg("http://kmail.kde.org/"));
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
    QString encoding;
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

  if (mViewer)
    if (mUnicodeFont)
      mViewer->setCharset("iso10646-1", true);
    else mViewer->setCharset(mCodec->name(), true);

  mViewer->write("<html><head><style type=\"text/css\">" +
		 QString("body { font-family: \"%1\" }\n").arg( mBodyFamily ) +
		 QString("p { font-size: %1pt }\n").arg( fntSize  ) +
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
  QString type, subtype, contDisp;
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
    if (!stricmp(msgPart.typeStr(), "text")
       && !stricmp(msgPart.subtypeStr(), "x-vcard")) {
        int vcerr;
        vc = VCard::parseVCard(msgPart.body(), &vcerr);

        if (vc) {
          delete vc;
          kdDebug() << "FOUND A VALID VCARD" << endl;
          vcnum = j;
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
      kdDebug() << "Alternative message, type: " << type.data() << endl;
      //Now: Only two attachments one of them is html
      for (i=0; i<2; i++)                   // count parts...
      {
        aMsg->bodyPart(i, &msgPart);        // set part...
        subtype = msgPart.subtypeStr();     // get subtype...
        if (htmlMail() && stricmp(subtype, "html")==0)    // is it html?
        {                                   // yes...
          str = msgPart.bodyDecoded();      // decode it...
          mViewer->write(mCodec->toUnicode(str.data()));    // write it...
          return;                           // return, finshed.
        }
	else if (!htmlMail() && (stricmp(subtype, "plain")==0))
	                                    // wasn't html show only if
	{                                   // support for html is turned off
          str = msgPart.bodyDecoded();      // decode it...
          writeBodyStr(str.data());
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
      kdDebug() << "type: " << type.data() << endl;
      kdDebug() << "subtye: " << subtype.data() << endl;
      kdDebug() << "contDisp " << contDisp.data() << endl;

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
//	if (i<=0 || stricmp(type, "text")==0)//||stricmp(type, "message")==0)
//	if (stricmp(type, "text")==0)//||stricmp(type, "message")==0)
	if ((type == "") || (stricmp(type, "text")==0))
	{
          QCString cstr(msgPart.bodyDecoded());
	  if (i>0)
	      mViewer->write("<br><hr><br>");

	  if (htmlMail() && (stricmp(subtype, "html")==0))
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
            mViewer->write(mCodec->toUnicode(cstr.data()));
	  }
          else writeBodyStr(cstr);
	}
        // ---Sven's view smart or inline image attachments in kmail start---
        else if (stricmp(type, "image")==0)
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
      writeBodyStr(aMsg->bodyDecoded());
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeMsgHeader(int vcpartnum)
{
  QString str;
  QString vcname;
  KMMessagePart aMsgPart;
  QString vcFileName;

  if (vcpartnum >= 0) {
    mMsg->bodyPart(vcpartnum, &aMsgPart);
    vcFileName = aMsgPart.fileName();
    if (vcFileName.isEmpty()) {
      vcFileName = "/unnamed";
    } else {
      // remove quotes from the filename so that the shell does not get confused
      int c = 0;
      while ((c = vcFileName.find('"', c)) >= 0)
        vcFileName.remove(c, 1);

      c = 0;
      while ((c = vcFileName.find('\'', c)) >= 0)
        vcFileName.remove(c, 1);
    }
    vcname = QString("%1part%2/%3").arg(mAttachDir).arg(vcpartnum+1).arg(vcFileName);
  }

  switch (mHeaderStyle)
  {
    case HdrBrief:
    mViewer->write("<font size=\"+1\"><b>" + strToHtml(mMsg->subject()) +
                   "</b></font>&nbsp; (" +
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
    mViewer->write("<font size=\"+1\"><b>" +
                   strToHtml(mMsg->subject()) + "</b></font><br>\n");
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
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<br>");
#endif
    mViewer->write("<br>");
    break;

  case HdrFancy:
    mViewer->write(QString("<table><tr><td><img src=") +
		   locate("data", "kmail/pics/kdelogo.xpm") +
                   "></td><td hspace=\"50\"><b><font size=\"+2\">");
    mViewer->write(strToHtml(mMsg->subject()) + "</font></b><br>");
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
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<br><br>\n");
#endif
    mViewer->write("</b></td></tr></table><br>\n");
    break;

  case HdrLong:
    mViewer->write("<font size=\"+1\"><b>" +
                   strToHtml(mMsg->subject()) + "</B></font><br>");
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
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ")+
                       KMMessage::refsAsAnchor(mMsg->references()) + "<br>\n");
    if (!mMsg->groups().isEmpty())
        mViewer->write(i18n("Groups: ") + mMsg->groups()+"<br>\n");
#endif
    mViewer->write("<br>\n");
    break;

  case HdrAll:
    str = strToHtml(mMsg->headerAsString());
    mViewer->write(str);
    mViewer->write("\n<br>\n");
    if (vcpartnum >= 0) {
      mViewer->write("<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    mViewer->write("\n<br>\n");
    break;

  default:
    qWarning("Unsupported header style %d", mHeaderStyle);
  }
  mViewer->write("<br>\n");
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr(const QCString aStr)
{
  QString line, sig, htmlStr = "";
  Kpgp* pgp = Kpgp::getKpgp();
  assert(pgp != NULL);
  // assert(!aStr.isNull());
  bool pgpMessage = false;

  if (pgp->setMessage(aStr))
  {
    QString str = pgp->frontmatter();
    if(!str.isEmpty()) htmlStr += mCodec->toUnicode(quotedHTML(str));
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

      /* HTMLize signedBy data */
      QString sdata=pgp->signedBy();
      sdata.replace(QRegExp("\""), "&quot;");
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
      htmlStr += mCodec->toUnicode(quotedHTML(pgp->message()));
      htmlStr += QString("<br><b>%1</b><br><br>")
        .arg(i18n("End of pgp message"));
      str = pgp->backmatter();
      if(!str.isEmpty()) htmlStr += mCodec->toUnicode(quotedHTML(str));
    } // if (!pgpMessage) then the message only looked similar to a pgp message
    else htmlStr = mCodec->toUnicode(quotedHTML(aStr));
  }
//  Mail header charset(iso-2022-jp) is using all most E-mail system in Japan.
//  ISO-2022-JP code consists of ESC(0x1b) character and 7Bit character which
//  used from '!' character to  '~' character.
//  JIS7 is header charset of iso-2022-jp.  toyo
  else if( QString(mCodec->name()) == "JIS7" )
    htmlStr += quotedHTML(mCodec->toUnicode(aStr));
  else htmlStr += mCodec->toUnicode(quotedHTML(aStr));
  mViewer->write(htmlStr);
}




//-----------------------------------------------------------------------------

QString KMReaderWin::quotedHTML(const QString& s)
{
  QString htmlStr, line, tmpStr;
  QChar ch;

  bool atStartOfLine;
  int pos, beg;

  int currQuoteLevel = -1;
  int prevQuoteLevel = -1;
  int newlineCount = 0;
  tmpStr = "<div>"; //work around KHTML slowness

  // skip leading empty lines
  for( pos = 0; pos < (int)s.length() && s[pos] <= ' '; pos++ );
  while (pos > 0 && s[pos-1] == ' ') pos--;
  beg = pos;

  atStartOfLine = TRUE;
  while( pos < (int)s.length() )
  {
    ch = s[pos];
    if(( ch == '\n' ) || ((unsigned)pos == s.length() - 1))
    {
      int adj = (ch == '\n') ? 0 : 1;
      newlineCount ++;
      line = strToHtml(s.mid(beg,pos-beg+adj),FALSE,TRUE);
      if( currQuoteLevel >= 0 )
      {
	if( currQuoteLevel != prevQuoteLevel )
	{
	  line.prepend( mQuoteFontTag[currQuoteLevel%3] );
	  if( prevQuoteLevel >= 0 )
	  {
	    line.prepend( "</font>" );
	  }
	}
	prevQuoteLevel = currQuoteLevel;
      }
      else if( prevQuoteLevel >= 0 )
      {
	line.prepend( "</font><br>\n" ); // Added extra BR to work around bug
	prevQuoteLevel = -1;
      }

      tmpStr += line + "<br>\n";
      if( (newlineCount % 100) == 0 )
      {
	htmlStr += tmpStr;
	if (currQuoteLevel >= 0)
	  htmlStr += "</font>";
	htmlStr += "</div><div>"; //work around KHTML slowness
	if (currQuoteLevel >= 0)
	  htmlStr += mQuoteFontTag[currQuoteLevel%3];
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

  htmlStr += tmpStr;
  htmlStr += "</div>"; //work around KHTML slowness
  return htmlStr;
}



//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum)
{
  QString iconName, href, label, comment, contDisp;
  QString fileName;

  if(aMsgPart == NULL) {
    kdDebug() << "writePartIcon: aMsgPart == NULL\n" << endl;
    return;
  }

  kdDebug() << "writePartIcon: PartNum: " << aPartNum << endl;

  comment = aMsgPart->contentDescription();

  fileName = aMsgPart->fileName();
  if (fileName.isEmpty()) fileName = aMsgPart->name();
  label = fileName;
      /* HTMLize label */
  label.replace(QRegExp("\""), "&quot;");
  label.replace(QRegExp("<"), "&lt;");
  label.replace(QRegExp(">"), "&gt;");

//--- Sven's save attachments to /tmp start ---
  bool ok = true;

  QString fname = QString("%1part%2").arg(mAttachDir).arg(aPartNum+1);
  if (access(fname.data(), W_OK) != 0) // Not there or not writable
    if (mkdir(fname.data(), 0) != 0 || chmod (fname.data(), S_IRWXU) != 0)
      ok = false; //failed create

  if (ok)
  {
    if (fileName.isEmpty())
      fname += "/unnamed";
    else
    {
      fname = fname + "/" + fileName;
      // remove quotes from the filename so that the shell does not get confused
      int c = 0;
      while ((c = fname.find('"', c)) >= 0)
	fname.remove(c, 1);

      c = 0;
      while ((c = fname.find('\'', c)) >= 0)
	fname.remove(c, 1);
    }

    // fixme: use KTempFile!
    if (!kByteArrayToFile(aMsgPart->bodyDecodedBinary(), fname, false, false, false))
      ok = false;
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
  if (iconName.left(11)=="unknown")
  {
    aMsgPart->magicSetType();
    iconName = aMsgPart->iconName();
  }
  mViewer->write("<table><tr><td><a href=\"" + href + "\"><img src=\"" +
		 iconName + "\" border=\"0\">" + label +
		 "</a></td></tr></table>" + comment + "<br>");
}


//-----------------------------------------------------------------------------
const QString KMReaderWin::strToHtml(const QString aStr, bool aDecodeQP,
				     bool aPreserveBlanks) const
{
  QCString qpstr, iStr, result;
  const char *pos;
  char ch, str[256];
  int i, i1, x, len;
  const int maxLen = 30000;
  char htmlStr[maxLen+256];
  char* htmlPos;
  bool startOfLine = true;

  // FIXME: use really unicode within a QString instead of utf8
  if (aDecodeQP) qpstr = KMMsgBase::decodeRFC2047String(aStr).utf8();
  else qpstr = aStr.utf8();

#define HTML_ADD(str,len) strcpy(htmlPos,str),htmlPos+=len

  htmlPos = htmlStr;
  for (pos=qpstr.data(),x=0; *pos; pos++,x++)
  {
    if ((int)(htmlPos-htmlStr) >= maxLen)
    {
      *htmlPos = '\0';
      result += htmlStr;
      htmlPos = htmlStr;
    }

    ch = *pos;
    if (aPreserveBlanks)
    {
      if (ch==' ')
      {
	if (startOfLine) {
	  HTML_ADD("&nbsp;", 6);
	  pos++, x++;
	  startOfLine = false;
	}
        while (*pos==' ')
        {
	  HTML_ADD(" ", 1);
          pos++, x++;
	  if (*pos==' ') {
	    HTML_ADD("&nbsp;", 6);
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
	  HTML_ADD("&nbsp;", 6);
	  x++;
	}
	while((x&7) != 0);
      }
    }
    if (ch=='<') HTML_ADD("&lt;", 4);
    else if (ch=='>') HTML_ADD("&gt;", 4);
    else if (ch=='\n') {
      HTML_ADD("<br>", 4);
      startOfLine = true;
    }
    else if (ch=='&') HTML_ADD("&amp;", 5);
    else if ((ch=='h' && strncmp(pos,"http:", 5)==0) ||
	     (ch=='h' && strncmp(pos,"https:", 6)==0) ||
	     (ch=='f' && strncmp(pos,"ftp:", 4)==0) ||
	     (ch=='m' && strncmp(pos,"mailto:", 7)==0))
    {
      for (i=0; *pos && *pos > ' ' && *pos != '\"' &&
                *pos != '<' &&		// handle cases like this: <link>http://foobar.org/</link>
		i < 255; i++, pos++)
	str[i] = *pos;
      pos--;
      while (i>0 && ispunct(str[i-1]) && str[i-1]!='/')
      {
	i--;
	pos--;
      }
      str[i] = '\0';
      HTML_ADD("<a href=\"", 9);
      HTML_ADD(str, strlen(str));
      HTML_ADD("\">", 2);
      HTML_ADD(str, strlen(str));
      HTML_ADD("</a>", 4);
    }
    else if (ch=='@')
    {
      const char *startofstring = qpstr.data();
      const char *startpos = pos;
      for (i=0; pos >= startofstring && *pos
	     && (isalnum(*pos)
		 || *pos=='@' || *pos=='.' || *pos=='_' || *pos=='-'
		 || *pos=='*' || *pos=='[' || *pos==']' || *pos=='=')
	     && i<255; i++, pos--)
	{
	}
      i1 = i;
      pos++;
      for (i=0; *pos && (isalnum(*pos)||*pos=='@'||*pos=='.'||
			 *pos=='_' || *pos=='-' || *pos=='*'  || *pos=='[' ||
                         *pos==']' || *pos=='=')
	     && i<255; i++, pos++)
      {
	iStr += *pos;
      }
      pos--;
      len = iStr.length();
      while (len>2 && ispunct(*pos) && (pos > startpos))
      {
	len--;
	pos--;
      }
      iStr.truncate(len);

      htmlPos -= (i1 - 1);
      if (iStr.length()>3 && iStr.at(0) != '@' 
        && iStr.at(iStr.length() - 1) != '@')
          iStr = "<a href=\"mailto:" + iStr + "\">" + iStr + "</a>";
      HTML_ADD(iStr.data(), iStr.length());
      iStr = "";
    }

    else {
      *htmlPos++ = ch;
      startOfLine = false;
    }
  }

  *htmlPos = '\0';
  result += htmlStr;
  return QString::fromUtf8(result);
}


//-----------------------------------------------------------------------------
void KMReaderWin::printMsg(void)
{
  if (!mMsg) return;
  mViewer->view()->print();
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const KURL &aUrl)
{
  if (aUrl.isEmpty() || !mMsg) return -1;

  if (!aUrl.isLocalFile()) return -1;

  QString prefix = mAttachDir + "part";

  if (aUrl.path().left(prefix.length()) == prefix)
  {
    QString num = aUrl.path().mid(prefix.length());
    int i = num.find('/');
    if (i > 0)
       num = num.left(i);

    return num.toInt();
  }
  return -1;
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
    mMsg->bodyPart(id-1, &msgPart);
    QString str = msgPart.fileName();
    if (str.isEmpty()) str = msgPart.name();
    emit statusMsg(i18n("Attachment: ") + str);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KURL &aUrl, const KParts::URLArgs &)
{
  int id = msgPartFromUrl(aUrl);
  if (id > 0)
  {
    // clicked onto an attachment
    mAtmCurrent = id-1;

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
    mAtmCurrent = id-1;
    KPopupMenu *menu = new KPopupMenu();
    menu->insertItem(i18n("Open..."), this, SLOT(slotAtmOpen()));
    menu->insertItem(i18n("Open with..."), this, SLOT(slotAtmOpenWith()));
    menu->insertItem(i18n("View..."), this, SLOT(slotAtmView()));
    menu->insertItem(i18n("Save as..."), this, SLOT(slotAtmSave()));
    //menu->insertItem(i18n("Print..."), this, SLOT(slotAtmPrint()));
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

  if (aReaderWin && stricmp(aMsgPart->typeStr(), "message")==0)
  {
    aReaderWin->atmViewMsg(aMsgPart);
    return;
  }

  kernel->kbp()->busy();
  {
    KMReaderWin* win = new KMReaderWin; //new reader
    if (stricmp(aMsgPart->typeStr(), "message")==0)
    {               // if called from compose win
      KMMessage* msg = new KMMessage;
      assert(aMsgPart!=NULL);
      msg->fromString(aMsgPart->bodyDecoded());
      win->setCaption(msg->subject());
      win->setMsg(msg, true);
      win->show();
    }
    else if (stricmp(aMsgPart->typeStr(), "text")==0)
    {
      if (stricmp(aMsgPart->subtypeStr(), "x-vcard") == 0) {
        KMDisplayVCard *vcdlg;
	int vcerr;
	VCard *vc = VCard::parseVCard(aMsgPart->body(), &vcerr);

	if (!vc) {
          QString errstring = i18n("Error reading in vCard:\n");
	  errstring += VCard::getError(vcerr);
	  KMessageBox::error(NULL, i18n(errstring), i18n("vCard error"));
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
      if (aHTML && (stricmp(aMsgPart->subtypeStr(), "html")==0))  // HTML
	win->mViewer->write(win->codec()->toUnicode(str));
      else  // plain text
	win->writeBodyStr(str);
      win->mViewer->write("</body></html>");
      win->mViewer->end();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    }
    else if (stricmp(aMsgPart->typeStr(), "image")==0)
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
      win->mViewer->write(linkName.data());
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
  atmView(this, &msgPart, htmlMail(), QString("%1part%2/%3").arg(mAttachDir).
    arg(mAtmCurrent+1).arg(pname), pname, mCodec);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  QString str, pname, cmd, fileName;
  KMMessagePart msgPart;

  mMsg->bodyPart(mAtmCurrent, &msgPart);
  if (stricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  if (stricmp(msgPart.typeStr(), "text") == 0)
  {
    if (stricmp(msgPart.subtypeStr(), "x-vcard") == 0) {
      KMDisplayVCard *vcdlg;
      int vcerr;
      VCard *vc = VCard::parseVCard(msgPart.body(), &vcerr);

      if (!vc) {
        QString errstring = i18n("Error reading in vCard:\n");
        errstring += VCard::getError(vcerr);
        KMessageBox::error(this, i18n(errstring), i18n("vCard error"));
        return;
      }

      vcdlg = new KMDisplayVCard(vc);
      vcdlg->show();
      return;
    }
  }

  fileName = getAtmFilename(msgPart.fileName(), msgPart.name());

  // What to do when user clicks on an attachment --dnaber, 2000-06-01
  // TODO: show full path for Service, not only name
  QString mimetype = KMimeType::findByURL(KURL(fileName))->name();
  KService::Ptr offer = KServiceTypeProfile::preferredService(mimetype, true);
  QString question;
  QString open_text = i18n("Open");
  if ( offer ) {
    question = i18n("Open attachment '%1' using '%2'?").arg(msgPart.fileName()).arg(offer->name());
  } else {
    question = i18n("Open attachment?");
    open_text = i18n("Open with...");
  }
  question += i18n("\n\nNote that opening an attachment may compromise your system's security!");
  // TODO: buttons don't have the correct order, but "Save" should be default
  int choice = KMessageBox::warningYesNoCancel(this, question,
      i18n("Open Attachment?"), i18n("Save to disk"), open_text, i18n("Cancel"));
  if( choice == KMessageBox::Yes ) {		// Save
    slotAtmSave();
  } else if( choice == KMessageBox::No ) {	// Open
    if ( offer ) {
      // There's a default service for this kind of file - use it
      KURL::List lst;
      KURL url;
      url.setPath(fileName);
      lst.append(url);
      KRun::run(*offer, lst);
    } else {
      // There's no know service that handles this type of file, so open
      // the "Open with..." dialog.
      KFileOpenWithHandler *openhandler = new KFileOpenWithHandler();
      KURL::List lst;
      KURL url;
      url.setPath(fileName);
      lst.append(url);
      openhandler->displayOpenWithDialog(lst);
    }
  } else {					// Cancel
    kdDebug() << "Canceled opening attachment" << endl;
  }

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpenWith()
{
  // It makes sense to have an extra "Open with..." entry in the menu
  // so the user can change filetype associations.

  KMMessagePart msgPart;

  mMsg->bodyPart(mAtmCurrent, &msgPart);
  QString fileName = getAtmFilename(msgPart.fileName(), msgPart.name());

  KFileOpenWithHandler *openhandler = new KFileOpenWithHandler();
  KURL::List lst;
  KURL url;
  url.setPath(fileName);
  lst.append(url);
  openhandler->displayOpenWithDialog(lst);
}


//-----------------------------------------------------------------------------
QString KMReaderWin::getAtmFilename(QString pname, QString msgpartname) {
  if (pname.isEmpty()) pname=msgpartname;
  if (pname.isEmpty()) pname="unnamed";
  QString fileName = QString("%1part%2/%3")
             .arg(mAttachDir).arg(mAtmCurrent+1).arg(pname);

  // remove quotes from the filename so that the shell does not get confused
  int c = 0;
  while ((c = fileName.find('"', c)) >= 0)
    fileName.remove(c, 1);

  c = 0;
  while ((c = fileName.find('\'', c)) >= 0)
    fileName.remove(c, 1);

  return fileName;
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

  KURL url = KFileDialog::getSaveURL( fileName, "*", this );

  if( url.isEmpty() )
    return;

  if( !url.isLocalFile() )
  {
    KMessageBox::sorry( 0L, i18n( "Only local files supported yet." ) );
    return;
  }

  fileName = url.path();
  mSaveAttachDir = url.directory() + "/";

  kernel->kbp()->busy();
  if (!kByteArrayToFile(msgPart.bodyDecodedBinary(), fileName, TRUE))
    qWarning(i18n("Could not save file"));
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmPrint()
{
  KMMessagePart msgPart;
  mMsg->bodyPart(mAtmCurrent, &msgPart);

  qWarning("KMReaderWin::slotAtmPrint()\nis not implemented");
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
}


//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}

//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"
