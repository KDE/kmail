// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include <qdir.h>
#include <kfiledialog.h>
#include <stdlib.h>
#include <sys/stat.h>

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
#include "kfontutils.h"
#include "kurl.h"

#include <khtml.h>
#include <kapp.h>
#include <kconfig.h>
#include <kcursor.h>
#include <krun.h>
#include <kmessagebox.h>
#include <mimelib/mimepp.h>
#include <qstring.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <qbitmap.h>
#include <qclipboard.h>
#include <qcursor.h>
#include <qmultilineedit.h>
#include <qregexp.h>
#include <qscrollbar.h>

// for selection
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

//--- Sven's save attachments to /tmp start ---
#include <unistd.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>  // for access and getpid
//--- Sven's save attachments to /tmp end ---

// Do the tmp stuff correctly - thanks to Harri Porten for
// reminding me (sven)

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifdef KRN
extern KApplication *app;
extern KBusyPtr *kbp;
#endif

QString KMReaderWin::mAttachDir;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent, const char *aName, int aFlags)
  :KMReaderWinInherited(aParent, aName, aFlags)
{
  initMetaObject();

  mAutoDelete = FALSE;
  mMsg = 0;

  initHtmlWidget();
  readConfig();

  if (mAttachDir.isNull()) makeAttachDir();
  connect(&updateReaderWinTimer, SIGNAL(timeout()),
  	  this,SLOT(updateReaderWin()));
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  if (mAutoDelete) delete mMsg;
}


//-----------------------------------------------------------------------------
void KMReaderWin::makeAttachDir(void)
{
  QString directory;
  directory.sprintf("kmail/tmp/kmail%d/", getpid());
  KGlobal::dirs()->
    addResourceType("kmail_tmp", 
		    KStandardDirs::kde_default("data") + directory);
  mAttachDir = locateLocal( "kmail_tmp", "/" );
  
  if (mAttachDir.isNull()) warning(i18n("Failed to create temporary "
					"attachment directory '%s': %s"), 
				   directory.ascii(), strerror(errno));
}


//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = kapp->config();

  config->setGroup("Pixmaps");
  mBackingPixmapOn = FALSE;
  mBackingPixmapStr = config->readEntry("Readerwin","");
  if (mBackingPixmapStr != "")
    mBackingPixmapOn = TRUE;
  
  config->setGroup("Reader");
  mAtmInline = config->readNumEntry("attach-inline", 100);
  mHeaderStyle = (HeaderStyle)config->readNumEntry("hdr-style", HdrFancy);
  mAttachmentStyle = (AttachmentStyle)config->readNumEntry("attmnt-style",
							   SmartAttmnt);
#ifdef KRN
  config->setGroup("ArticleListOptions");
#endif
  c1 = QColor(app->palette().normal().text());
  c2 = QColor("blue");
  c3 = QColor("red");
  c4 = QColor(app->palette().normal().base());

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    c4 = config->readColorEntry("BackgroundColor",&c4);
    c1 = config->readColorEntry("ForegroundColor",&c1);
    c2 = config->readColorEntry("LinkColor",&c2);
    c3 = config->readColorEntry("FollowedColor",&c3);
    mViewer->setDefaultBGColor(c4);
    mViewer->setDefaultTextColors(c1,c2,c3);
  }
  else {
    mViewer->setDefaultBGColor(c4);
    mViewer->setDefaultTextColors(c1,c2,c3);
  }
  
#ifndef KRN
  int i, diff;
  fntSize = 0;
  // --- sven's get them font sizes right! start ---
  config->setGroup("Fonts");
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    mBodyFont = config->readEntry("body-font", "helvetica-medium-r-12");
    mViewer->setStandardFont(kstrToFont(mBodyFont).family());
    fntSize = kstrToFont(mBodyFont).pointSize();
    mBodyFamily = kstrToFont(mBodyFont).family();
  }
  else {
    setFont(KGlobal::generalFont());
    fntSize = KGlobal::generalFont().pointSize();
    mBodyFamily = KGlobal::generalFont().family();
  }

  int fontsizes[7];
  mViewer->resetFontSizes();
  mViewer->fontSizes(fontsizes);
  diff= fntSize - fontsizes[3];
  if (fontsizes[0]+diff > 0)
    for (i=0;i<7; i++)
      fontsizes[i]+=diff;

  mViewer->setFontSizes(fontsizes);
  // --- sven's get them font sizes right! end ---
  //mViewer->setFixedFont(mFixedFont);
#else
  mViewer->setDefaultFontBase(config->readNumEntry("DefaultFontBase",3));
  mViewer->setStandardFont(config->readEntry("StandardFont","helvetica"));
  mViewer->setFixedFont(config->readEntry("FixedFont","courier"));
#endif
  if (mMsg)
    update();
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig(bool aWithSync)
{
  KConfig *config = kapp->config();

  config->setGroup("Reader");
  config->writeEntry("attach-inline", mAtmInline);
  config->writeEntry("hdr-style", (int)mHeaderStyle);
  config->writeEntry("attmnt-style",(int)mAttachmentStyle);
  
  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  mViewer = new KHTMLWidget(this, "khtml");
  mViewer->resize(width()-16, height()-110);
  mViewer->setURLCursor(KCursor::handCursor());
  mViewer->setDefaultBGColor(QColor("#ffffff"));
  mViewer->setFollowsLinks( FALSE );

  // ### FIXME
  connect(mViewer,SIGNAL(urlClicked(const QString& , const QString &, int)),this,
  	  SLOT(slotUrlOpen(const QString &, const QString &, int)));
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
	  SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
	  SLOT(slotUrlPopup(const QString &, const QPoint &)));
  connect(mViewer,SIGNAL(textSelected(bool)),
          SLOT(slotTextSelected(bool)));

  // ### FIXME
  //connect(mViewer, SIGNAL(documentChanged()), SLOT(slotDocumentChanged()));
  //connect(mViewer, SIGNAL(documentDone()), SLOT(slotDocumentDone()));
}


//-----------------------------------------------------------------------------
void KMReaderWin::setBodyFont(const QString aFont)
{
  mBodyFont = aFont.copy();
  update(true);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHeaderStyle(KMReaderWin::HeaderStyle aHeaderStyle)
{
  mHeaderStyle = aHeaderStyle;
  update(true);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setAttachmentStyle(int aAttachmentStyle)
{  
  mAttachmentStyle = (AttachmentStyle)aAttachmentStyle;
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

  // Avoid flicker, somewhat of a cludge
  if (force) {
    mMsgBuf = 0;
    updateReaderWin();
  }
  else if (updateReaderWinTimer.isActive())
    updateReaderWinTimer.changeInterval( 100 );
  else {
    //    updateReaderWin();
    updateReaderWinTimer.start( 0, TRUE );  
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
  if (mMsgBuf == mMsg)
    return;

  if (mMsg) parseMsg();
  else
  {
    mViewer->begin();
    mViewer->write("<HTML><BODY" +
		   QString(" TEXT=#%1").arg(colorToString(c1)) +
		   QString(" LINK=#%1").arg(colorToString(c2)) +
		   QString(" VLINK=#%1").arg(colorToString(c3)) +
		   QString(" BGCOLOR=#%1").arg(colorToString(c4)));

    if (mBackingPixmapOn)
      mViewer->write(" background=\"file://" + mBackingPixmapStr + "\"");
    mViewer->write("></BODY></HTML>");
    mViewer->end();
  }

  mMsgBuf = mMsg;
}

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

  mViewer->begin();
  mViewer->write("<HTML><BODY" +
		 QString(" TEXT=#%1").arg(colorToString(c1)) +
		 QString(" LINK=#%1").arg(colorToString(c2)) +
		 QString(" VLINK=#%1").arg(colorToString(c3)) +
		 QString(" BGCOLOR=#%1").arg(colorToString(c4)));

  if (mBackingPixmapOn)
    mViewer->write(" background=\"file://" + mBackingPixmapStr + "\"");
  mViewer->write("><FONT FACE=\"" + mBodyFont + 
		 //		 QString(" SIZE=\"%1\"").arg(fntSize) + 
		 "\">");

#if defined CHARSETS  
  printf("Setting viewer charset to %s\n",(const char *)mMsg->charset());
  mViewer->setCharset(mMsg->charset());
#endif  

  parseMsg(mMsg);

  mViewer->write("</FONT></BODY></HTML>");
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
  KMMessagePart msgPart;
  int i, numParts;
  QString type, subtype, str, contDisp;
  bool asIcon = false;
  inlineImage = false;
  
  assert(aMsg!=NULL);
  type = aMsg->typeStr();
  numParts = aMsg->numBodyParts();

  writeMsgHeader();
  if (numParts > 0)
  {
    // ---sven: handle multipart/alternative start ---
    // This is for multipart/alternative messages WITHOUT attachments
    // main header has type=multipart/alternative and one attachment is
    // text/html
    if (type.find("multipart/alternative") != -1 && numParts == 2)
    {
      debug("Alternative message, type: %s",type.data());
      //Now: Only two attachments one of them is html
      for (i=0; i<2; i++)                   // count parts...
      {
        aMsg->bodyPart(i, &msgPart);        // set part...
        subtype = msgPart.subtypeStr();     // get subtype...
        if (stricmp(subtype, "html")==0)    // is it html?
        {                                   // yes...
          str = QCString(msgPart.bodyDecoded());      // decode it...
          mViewer->write(str);              // write it...
          return;                           // return, finshed.
        }                                   // wasn't html ignore.
      }                                     // end for.
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
      debug("type: %s",type.data());
      debug("subtye: %s",subtype.data());
      debug("contDisp %s",contDisp.data());
      
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
	if (stricmp(type, "text")==0)//||stricmp(type, "message")==0)
	{
	  str = QCString(msgPart.bodyDecoded());
	  if (i>0) mViewer->write("<BR><HR><BR>");

	  if (stricmp(subtype, "html")==0) 
          {
            // ---Sven's strip </BODY> and </HTML> from end of attachment start-
            // We must fo this, or else we will see only 1st inlined html attachment
            // It is IMHO enough to search only for </BODY> and put \0 there.
            int i;
            i = str.findRev("</body>", -1, false); //case insensitive
            if (i>0)
              str.truncate(i);
            else // just in case - search for </html>
            {
              i = str.findRev("</html>", -1, false); //case insensitive
              if (i>0) str.truncate(i);
            }
            // ---Sven's strip </BODY> and </HTML> from end of attachment end-
            mViewer->write(str);
	  }
          else writeBodyStr(str);
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
    if (type.find("text/html;") != -1)
      mViewer->write(aMsg->bodyDecoded());
    else
      writeBodyStr(aMsg->bodyDecoded());
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeMsgHeader(void)
{
  QString str;

  switch (mHeaderStyle)
  {
    case HdrBrief:
    mViewer->write("<FONT SIZE=+1><B>" + strToHtml(mMsg->subject()) + 
                   "</B></FONT>&nbsp; (" +
                   KMMessage::emailAddrAsAnchor(mMsg->from(),TRUE) + ", ");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->cc(),TRUE) + ", ");
    mViewer->write(strToHtml(mMsg->dateShortStr()) + ")<BR>\n");
    break;

  case HdrStandard:
    mViewer->write("<FONT SIZE=+1><B>" +
                   strToHtml(mMsg->subject()) + "</B></FONT><BR>\n");
    mViewer->write(i18n("From: ") +
                   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE) + "<BR>\n");
    mViewer->write(i18n("To: ") +
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<BR>\n");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<BR>\n");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR>");
#endif
    mViewer->write("<BR>");
    break;

  case HdrFancy:
    mViewer->write(QString("<TABLE><TR><TD><IMG SRC=") + 
		   locate("data", "kmail/pics/kdelogo.xpm") +
                   "></TD><TD HSPACE=50><B><FONT SIZE=+2>");
    mViewer->write(strToHtml(mMsg->subject()) + "</FONT></B><BR>");
    mViewer->write(i18n("From: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE) + "<BR>\n");
    mViewer->write(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<BR>\n");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<BR>\n");
    mViewer->write(i18n("Date: ")+
                   strToHtml(mMsg->dateStr()) + "<BR>\n");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR><BR>\n");
#endif
    mViewer->write("</B></TD></TR></TABLE><BR>\n");
    break;

  case HdrLong:
    mViewer->write("<FONT SIZE=+1><B>" +
                   strToHtml(mMsg->subject()) + "</B></FONT><BR>");
    mViewer->write(i18n("Date: ")+strToHtml(mMsg->dateStr())+"<BR>");
    mViewer->write(i18n("From: ")+
		   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE) + "<BR>");
    mViewer->write(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<BR>");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<BR>");
    if (!mMsg->bcc().isEmpty())
      mViewer->write(i18n("Bcc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->bcc(),FALSE) + "<BR>");
    if (!mMsg->replyTo().isEmpty())
      mViewer->write(i18n("Reply to: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->replyTo(),FALSE) + "<BR>");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ")+
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR>\n");
    if (!mMsg->groups().isEmpty())
        mViewer->write(i18n("Groups: ") + mMsg->groups()+"<BR>\n");
#endif
    mViewer->write("<BR>\n");
    break;

  case HdrAll:
    str = strToHtml(mMsg->headerAsString());
    mViewer->write(str);
    mViewer->write("\n<BR>\n");
    break;

  default:
    warning("Unsupported header style %d", mHeaderStyle);
  }
  mViewer->write("<BR>\n");
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr(const QString aStr)
{
  QString line, sig, htmlStr = "";
  Kpgp* pgp = Kpgp::getKpgp();
  assert(pgp != NULL);
  //  assert(!aStr.isNull());
  bool pgpMessage = false;

  if (pgp->setMessage(aStr))
  {
    QString str = pgp->frontmatter();
    if(!str.isEmpty()) htmlStr += quotedHTML(str);
    htmlStr += "<BR>";
    if (pgp->isEncrypted())
    {      
      pgpMessage = true;
      if(pgp->decrypt())
      {
	htmlStr += QString("<B>%1</B><BR>").arg(i18n("Encrypted message"));
      }
      else
      {
	htmlStr += QString("<B>%1</B><BR>%2<BR><BR>")
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
         htmlStr += QString("<B>%1 %2</B><BR>").arg(sig).arg(sdata);
      } 
      else {
         htmlStr += QString("<B>%1 <A HREF=\"mailto:%2\">%3</A></B><BR>")
                      .arg(sig).arg(sdata).arg(sdata);
      }
    }
    htmlStr += quotedHTML(pgp->message());
    if(pgpMessage) htmlStr += "<BR><B>End pgp message</B><BR><BR>";
    str = pgp->backmatter();
    if(!str.isEmpty()) htmlStr += quotedHTML(str);
  }
  else htmlStr += quotedHTML(aStr);

  mViewer->write(htmlStr);
}


//-----------------------------------------------------------------------------
QString KMReaderWin::quotedHTML(const QString& s)
{
  int pos = 0, beg = 0;
  QString htmlStr, line;
  QChar ch;
  bool quoted = FALSE;
  bool lastQuoted = FALSE;
  bool atStart = TRUE;

  htmlStr = "";

  // skip leading empty lines
  while ( pos < (int)s.length() && s[pos] <= ' ' )
    pos++;

  beg = pos;
  int tcnt = 0;
  QString tmpStr;

  while (pos < (int)s.length())
  {
    ch = s[pos];
    if (ch=='\n')
    {
      tcnt ++;
      line = strToHtml(s.mid(beg,pos-beg),TRUE,TRUE);
      if (quoted && !lastQuoted) line.prepend("<I>");
      else if (!quoted && lastQuoted) line.prepend("</I>");
      tmpStr += line + "<BR>\n";
      if (!(tcnt % 100)) {
	htmlStr += tmpStr;
	tmpStr.truncate(0);
      }
      beg = pos+1;
      atStart = TRUE;
      lastQuoted = quoted;
      quoted = FALSE;
    }
    else if (ch > ' ' && atStart)
    {
      if (ch=='>' || /*ch==':' ||*/ ch=='|') quoted = TRUE;
      atStart = FALSE;
    }
    pos++;
  }
  htmlStr += tmpStr;
  return htmlStr;
}

//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum)
{
  QString iconName, href, label, comment, contDisp;
  QString fileName;

  if(aMsgPart == NULL) {
    debug("writePartIcon: aMsgPart == NULL\n");
    return;
  }

  debug("writePartIcon: PartNum: %i",aPartNum);

  comment = aMsgPart->contentDescription();

  fileName = aMsgPart->fileName();
  if (fileName.isEmpty()) fileName = aMsgPart->name();
  label = fileName;

//--- Sven's save attachments to /tmp start ---
  bool ok = true;

  QString fname = QString("%1/part%2").arg(mAttachDir).arg(aPartNum+1);
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

    if (!kByteArrayToFile(aMsgPart->bodyDecoded(), fname, false, false, false))
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
  mViewer->write("<TABLE><TR><TD><A HREF=\"" + href + "\"><IMG SRC=\"" + 
		 iconName + "\" BORDER=0>" + label + 
		 "</A></TD></TR></TABLE>" + comment + "<BR>");
}


//-----------------------------------------------------------------------------
const QString KMReaderWin::strToHtml(const QString aStr, bool aDecodeQP,
				     bool aPreserveBlanks) const
{
  QString qpstr, iStr, result;
  const char *pos;
  char ch, str[256];
  int i, i1, x, len;
  int maxLen = 30000;
  char htmlStr[maxLen+256];
  char* htmlPos;

  if (aDecodeQP) qpstr = KMMsgBase::decodeRFC1522String(aStr);
  else qpstr = aStr;

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
        while (*pos==' ')
        {
          HTML_ADD("&nbsp;", 6);
          pos++, x++;
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
      else aPreserveBlanks = FALSE;
    }
    if (ch=='<') HTML_ADD("&lt;", 4);
    else if (ch=='>') HTML_ADD("&gt;", 4);
    else if (ch=='\n') HTML_ADD("<BR>", 4);
    else if (ch=='&') HTML_ADD("&amp;", 5);
    else if ((ch=='h' && strncmp(pos,"http:", 5)==0) ||
	     (ch=='f' && strncmp(pos,"ftp:", 4)==0) ||
	     (ch=='m' && strncmp(pos,"mailto:", 7)==0))
    {
      for (i=0; *pos && *pos>' ' && i<255; i++, pos++)
	str[i] = *pos;
      pos--;
      while (i>0 && ispunct(str[i-1]) && str[i-1]!='/')
      {
	i--;
	pos--;
      }
      str[i] = '\0';
      HTML_ADD("<A HREF=\"", 9);
      HTML_ADD(str, strlen(str));
      HTML_ADD("\">", 2);
      HTML_ADD(str, strlen(str));
      HTML_ADD("</A>", 4);
    }
    else if (ch=='@')
    {
      const char *startofstring = qpstr.data();
      const char *startpos = pos;
      for (i=0; pos >= startofstring && *pos 
	     && (isalnum(*pos) 
		 || *pos=='@' || *pos=='.' || *pos=='_'||*pos=='-' 
		 || *pos=='*' || *pos=='[' || *pos==']') 
	     && i<255; i++, pos--)
	{
	}
      i1 = i;
      pos++; 
      for (i=0; *pos && (isalnum(*pos)||*pos=='@'||*pos=='.'||
			 *pos=='_'||*pos=='-' || *pos=='*'  || *pos=='[' || *pos==']') 
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
      if (iStr.length()>3)
	iStr = "<A HREF=\"mailto:" + iStr + "\">" + iStr + "</A>";
      HTML_ADD(iStr.data(), iStr.length());
      iStr = "";
    }

    else *htmlPos++ = ch;
  }

  *htmlPos = '\0';
  result += htmlStr;
  return result;
}


//-----------------------------------------------------------------------------
void KMReaderWin::printMsg(void)
{
  if (!mMsg) return;
  // FIXME
  //mViewer->print();
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const char* aUrl)
{
  if (!aUrl || !mMsg) return -1;
  
  QString url = QString("file:%1/part").arg(mAttachDir);
  int s = url.length();
  if (strncmp(aUrl, url, s) == 0)
  {
    url = aUrl;
    int i = url.find('/', s);
    url = url.mid(s, i-s);
    //debug ("Url num = %s", url.data());
    return atoi(url.data());
  }
  return -1;
}


//-----------------------------------------------------------------------------
void KMReaderWin::resizeEvent(QResizeEvent *)
{
  mViewer->setGeometry(0, 0, width(), height());
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
  int id;
  KMMessagePart msgPart;
  QString str;
  QString url = aUrl;
  KURL::decode( url );

  id = msgPartFromUrl(url);
  if (id <= 0)
  {
    emit statusMsg(url);
  }
  else
  {
    mMsg->bodyPart(id-1, &msgPart);
    str = msgPart.fileName();
    if (str.isEmpty()) str = msgPart.name();
    emit statusMsg(i18n("Attachment: ") + str);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const QString &aUrl, const QString &, int aButton)
{
  int id;
  QString url = aUrl;
  KURL::decode( url );

  id = msgPartFromUrl(url);
  if (id > 0)
  {
    // clicked onto an attachment
    mAtmCurrent = id-1;

    slotAtmOpen();
  }
  else emit urlClicked(url, aButton);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  KMMessagePart msgPart;
  int id;
  QPopupMenu *menu;
  QString url = aUrl;
  KURL::decode( url );

  id = msgPartFromUrl(url);
  if (id <= 0) emit popupMenu(aUrl, aPos);
  else
  {
    // Attachment popup
    mAtmCurrent = id-1;
    menu = new QPopupMenu();
    menu->insertItem(i18n("Open..."), this, SLOT(slotAtmOpen()));
    menu->insertItem(i18n("View..."), this, SLOT(slotAtmView()));
    menu->insertItem(i18n("Save as..."), this, SLOT(slotAtmSave()));
    //menu->insertItem(i18n("Print..."), this, SLOT(slotAtmPrint()));
    menu->insertItem(i18n("Properties..."), this,
		     SLOT(slotAtmProperties()));
    menu->popup(aPos,0);
  }
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
void KMReaderWin::slotAtmView()
{
  QString str, pname;
  KMMessagePart msgPart;
  // ---Sven's view text, html and image attachments in html widget start ---
  // Sven commented out
  //QMultiLineEdit* edt = new QMultiLineEdit;
  // ---Sven's view text, html and image attachments in html widget end ---
  
  mMsg->bodyPart(mAtmCurrent, &msgPart);
  pname = msgPart.fileName();
  if (pname.isEmpty()) pname=msgPart.name();
  if (pname.isEmpty()) pname=msgPart.contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  if (stricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  kbp->busy();
  // ---Sven's view text, html and image attachments in html widget start ---
  // ***start commenting out old stuff
  //  str = QCString(msgPart.bodyDecoded());

  //edt->setCaption(i18n("View Attachment: ") + pname);
  //edt->insertLine(str);
  //edt->setReadOnly(TRUE);
  //edt->show();
  // *** end commenting out old stuff
  {

    KMReaderWin* win = new KMReaderWin; //new reader
    
    if (stricmp(msgPart.typeStr(), "text")==0)
    {
      win->mViewer->begin();
      win->mViewer->write("<HTML><BODY>");
      QString str = msgPart.bodyDecoded();
      if (stricmp(msgPart.subtypeStr(), "html")==0)
        win->mViewer->write(str);
      else  //plain text
        win->writeBodyStr(str);
      win->mViewer->write("</BODY></HTML>");
      win->mViewer->end();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    }
    else if (stricmp(msgPart.typeStr(), "image")==0)
    {
      //image
      // Attachment is saved already; this is the file:
      QString linkName = QString("<img src=\"file:%1/part%2/%3\" border=0>")
                        .arg(mAttachDir).arg(mAtmCurrent+1).arg(pname);
      win->mViewer->begin();
      win->mViewer->write("<HTML><BODY>");
      win->mViewer->write(linkName.data());
      win->mViewer->write("</BODY></HTML>");
      win->mViewer->end();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    }
  }
  // ---Sven's view text, html and image attachments in html widget end ---
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  QString str, pname, cmd, fileName;
  KMMessagePart msgPart;
  // char* tmpName;
  // int old_umask;
  int c;

  mMsg->bodyPart(mAtmCurrent, &msgPart);

  if (stricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  pname = msgPart.fileName();
  if (pname.isEmpty()) pname=msgPart.name();
  if (pname.isEmpty()) pname="unnamed";
  //--- Sven's save attachments to /tmp start ---
  // Sven added:
  fileName = QString("%1/part%2/%3")
             .arg(mAttachDir).arg(mAtmCurrent+1).arg(pname);
  // Sven commented out:
  //tmpName = tempnam(NULL, NULL);
  //if (!tmpName)
  //{
  //  warning(i18n("Could not create temporary file"));
  //  return;
  //}
  //fileName = tmpName;
  //free(tmpName);
  //fileName += '-';
  //fileName += pname;

  // remove quotes from the filename so that the shell does not get confused
  c = 0;
  while ((c = fileName.find('"', c)) >= 0)
    fileName.remove(c, 1);

  c = 0;
  while ((c = fileName.find('\'', c)) >= 0)
    fileName.remove(c, 1);
  
  // Sven commented out:
  //kbp->busy();
  // NOTE: this next line will not work with Qt 2.0 - use a QByteArray str.
  //str = msgPart.bodyDecoded();
  //old_umask = umask(077);
  //if (!kCStringToFile(str, fileName, TRUE))
  //  warning(i18n("Could not save temporary file %s"),
  //	    (const char*)fileName);
  //umask(old_umask);
  //kbp->idle();
  //--- Sven's save attachments to /tmp end ---

  // -- David : replacement for KFM::openURL
  (void) new KRun(fileName);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  KMMessagePart msgPart;
  QString fileName;
  fileName = QDir::currentDirPath();
  fileName.append("/");

  
  mMsg->bodyPart(mAtmCurrent, &msgPart);
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

  kbp->busy();
  if (!kByteArrayToFile(msgPart.bodyDecoded(), fileName, TRUE))
    warning(i18n("Could not save file"));
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmPrint()
{
  KMMessagePart msgPart;
  mMsg->bodyPart(mAtmCurrent, &msgPart);

  warning("KMReaderWin::slotAtmPrint()\nis not implemented");
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmProperties()
{
  KMMessagePart msgPart;
  KMMsgPartDlg  dlg(0,TRUE);

  kbp->busy();
  mMsg->bodyPart(mAtmCurrent, &msgPart);
  dlg.setMsgPart(&msgPart);
  kbp->idle();

  dlg.exec();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollUp()
{
  mViewer->scrollBy(0, -10);	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  mViewer->scrollBy(0, 10);	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollPrior()
{
  mViewer->scrollBy(0, -(int)(height()*0.8));	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollNext()
{
  mViewer->scrollBy(0, (int)(height()*0.8));	
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
#include "kmreaderwin.moc"
