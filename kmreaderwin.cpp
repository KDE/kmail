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

#include "kmimemagic.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmreaderwin.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include "kpgp.h"
#include "kfontutils.h"

#include <html.h>
#include <kapp.h>
#include <kconfig.h>
#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <qstring.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <qmlined.h>
#include <qregexp.h>


#define hand_width 16
#define hand_height 16

static unsigned char hand_bits[] = {
  0x00,0x00,0xfe,0x01,0x01,0x02,0x7e,0x04,0x08,0x08,0x70,0x08,0x08,0x08,0x70,
  0x14,0x08,0x22,0x30,0x41,0xc0,0x20,0x40,0x12,0x80,0x08,0x00,0x05,0x00,0x02,
  0x00,0x00};
static unsigned char hand_mask_bits[] = {
  0xfe,0x01,0xff,0x03,0xff,0x07,0xff,0x0f,0xfe,0x1f,0xf8,0x1f,0xfc,0x1f,0xf8,
  0x3f,0xfc,0x7f,0xf8,0xff,0xf0,0x7f,0xe0,0x3f,0xc0,0x1f,0x80,0x0f,0x00,0x07,
  0x00,0x02};

#ifdef KRN
extern KApplication *app;
extern KLocale *nls;
extern KBusyPtr *kbp;
#endif

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent, const char *aName, int aFlags)
  :KMReaderWinInherited(aParent, aName, aFlags)
{
  initMetaObject();

  mPicsDir = app->kde_datadir()+"/kmail/pics/";
  mAutoDelete = FALSE;
  mMsg = NULL;

  initHtmlWidget();
  readConfig();
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  if (mAutoDelete) delete mMsg;
}


//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = kapp->getConfig();

  config->setGroup("Reader");
  mAtmInline = config->readNumEntry("attach-inline", 100);
  mHeaderStyle = (HeaderStyle)config->readNumEntry("hdr-style", HdrFancy);
  mAttachmentStyle = (AttachmentStyle)config->readNumEntry("attmnt-style",
							SmartAttmnt);
#ifdef KRN
  config->setGroup("ArticleListOptions");
#endif
  QColor c1=QColor("black");
  QColor c2=QColor("blue");
  QColor c3=QColor("red");
  QColor c4=QColor("white");

  mViewer->setDefaultBGColor(config->readColorEntry("BackgroundColor",&c4));
  mViewer->setDefaultTextColors(config->readColorEntry("ForegroundColor",&c1)
                                ,config->readColorEntry("LinkColor",&c2)
                                ,config->readColorEntry("FollowedColor",&c3));
  mViewer->setDefaultFontBase(config->readNumEntry("DefaultFontBase",3));

#ifndef KRN
  config->setGroup("Fonts");
  mBodyFont = config->readEntry("body-font", "helvetica-medium-r-12");
  mViewer->setStandardFont(kstrToFont(mBodyFont).family());
  //mViewer->setFixedFont(mFixedFont);
#else
  mViewer->setStandardFont(config->readEntry("StandardFont","helvetica"));
  mViewer->setFixedFont(config->readEntry("FixedFont","courier"));
#endif
  update();
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig(bool aWithSync)
{
  KConfig *config = kapp->getConfig();

  config->setGroup("Reader");
  config->writeEntry("attach-inline", mAtmInline);
  config->writeEntry("hdr-style", (int)mHeaderStyle);
  config->writeEntry("attmnt-style",(int)mAttachmentStyle);

  config->setGroup("Fonts");
  config->writeEntry("body-font", mBodyFont);
  // config->writeEntry("fixed-font", mFixedFont);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  QBitmap handImg(hand_width, hand_height, hand_bits, TRUE);
  QBitmap handMask(hand_width, hand_height, hand_mask_bits, TRUE);
  QCursor handCursor(handImg, handMask, 0, 0);

  mViewer = new KHTMLWidget(this, mPicsDir);
  mViewer->resize(width()-16, height()-110);
  mViewer->setURLCursor(handCursor);
  mViewer->setDefaultBGColor(QColor("#ffffff"));
  /*
  mViewer->setDefaultBGColor(pal->normal().background());
  mViewer->setDefaultTextColor(app->textColor, app->);
  */
  connect(mViewer,SIGNAL(URLSelected(const char *,int)),this,
	  SLOT(slotUrlOpen(const char *,int)));
  connect(mViewer,SIGNAL(onURL(const char *)),this,
	  SLOT(slotUrlOn(const char *)));
  connect(mViewer,SIGNAL(popupMenu(const char *, const QPoint &)),  
	  SLOT(slotUrlPopup(const char *, const QPoint &)));

  mSbVert = new QScrollBar(0, 110, 12, height()-110, 0, 
			   QScrollBar::Vertical, this);
  mSbHorz = new QScrollBar(0, 0, 24, width()-32, 0,
			   QScrollBar::Horizontal, this);	
  connect(mViewer, SIGNAL(scrollVert(int)), SLOT(slotScrollVert(int)));
  connect(mViewer, SIGNAL(scrollHorz(int)), SLOT(slotScrollHorz(int)));
  connect(mSbVert, SIGNAL(valueChanged(int)), mViewer, SLOT(slotScrollVert(int)));
  connect(mSbHorz, SIGNAL(valueChanged(int)), mViewer, SLOT(slotScrollHorz(int)));
  connect(mViewer, SIGNAL(documentChanged()), SLOT(slotDocumentChanged()));
  connect(mViewer, SIGNAL(documentDone()), SLOT(slotDocumentDone()));
}


//-----------------------------------------------------------------------------
void KMReaderWin::setBodyFont(const QString aFont)
{
  mBodyFont = aFont.copy();
  update();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHeaderStyle(KMReaderWin::HeaderStyle aHeaderStyle)
{
  mHeaderStyle = aHeaderStyle;
  update();
  writeConfig(FALSE);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setAttachmentStyle(int aAttachmentStyle)
{  
  mAttachmentStyle = (AttachmentStyle)aAttachmentStyle;
  update();
  writeConfig(FALSE);
}

//-----------------------------------------------------------------------------
void KMReaderWin::setInlineAttach(int aAtmInline)
{
  mAtmInline = aAtmInline;
  update();
  writeConfig(FALSE);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMsg(KMMessage* aMsg)
{
  mMsg = aMsg;

  if (mMsg) parseMsg();
  else
  {
    mViewer->begin(mPicsDir);
    mViewer->write("<HTML><BODY></BODY></HTML>");
    mViewer->end();
    mViewer->parse();
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(void)
{
  assert(mMsg!=NULL);

  mViewer->begin(mPicsDir);
  mViewer->write("<HTML><BODY>");
#ifdef CHARSETS  
  printf("Setting viewer charset to %s\n",(const char *)mMsg->charset());
  mViewer->setCharset(mMsg->charset());
#endif  

  parseMsg(mMsg);

  mViewer->write("</BODY></HTML>");
  mViewer->end();
  mViewer->parse();
}


//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
  KMMessagePart msgPart;
  int i, numParts;
  QString type, subtype, str, contDisp;
  bool asIcon = false;

  assert(aMsg!=NULL);
  writeMsgHeader();

  numParts = aMsg->numBodyParts();
  if (numParts > 0)
  {
    for (i=0; i<numParts; i++)
    {
      aMsg->bodyPart(i, &msgPart);
      type = msgPart.typeStr();
      subtype = msgPart.subtypeStr();
      contDisp = msgPart.contentDisposition();
      
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
	if (i<=0 || stricmp(type, "text")==0)//||stricmp(type, "message")==0)
	{
	  str = msgPart.bodyDecoded();
	  if (i>0) mViewer->write("<BR><HR><BR>");

	  if (stricmp(subtype, "html")==0) mViewer->write(str);
	  else writeBodyStr(str);
	}
	else asIcon = TRUE;
      }
      if (asIcon)
      {
	writePartIcon(&msgPart, i);
      }
    }
  }
  else
  {
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
    mViewer->write(strToHtml(mMsg->dateShortStr()) + ")<BR>");
    break;

  case HdrStandard:
    mViewer->write("<FONT SIZE=+1><B>" +
		   strToHtml(mMsg->subject()) + "</B></FONT><BR>");
    mViewer->write(i18n("From: ") +
		   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE) + "<BR>");
    mViewer->write(i18n("To: ") +
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<BR>");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<BR>");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR>");
#endif
    mViewer->write("<BR>");
    break;

  case HdrFancy:
    mViewer->write(QString("<TABLE><TR><TD><IMG SRC=") + mPicsDir +
		   "kdelogo.xpm></TD><TD HSPACE=50><B><FONT SIZE=+2>");
    mViewer->write(strToHtml(mMsg->subject()) + "</FONT></B><BR>");
    mViewer->write(i18n("From: ")+
		   KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE) + "<BR>");
    mViewer->write(i18n("To: ")+
		   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<BR>");
    if (!mMsg->cc().isEmpty())
      mViewer->write(i18n("Cc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<BR>");
    mViewer->write(i18n("Date: ")+
		   strToHtml(mMsg->dateStr()) + "<BR>");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(i18n("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR><BR>");
#endif
    mViewer->write("</B></TD></TR></TABLE><BR>");
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
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR>");
    if (!mMsg->groups().isEmpty())
        mViewer->write(i18n("Groups: ") + mMsg->groups()+"<BR>");
#endif
    mViewer->write("<BR>");
    break;

  case HdrAll:
    str = strToHtml(mMsg->headerAsString());
    mViewer->write(str);
    mViewer->write("<BR><BR>");
    break;

  default:
    warning("Unsupported header style %d", mHeaderStyle);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr(const QString aStr)
{
  char ch, *pos, *beg;
  bool atStart = TRUE;
  bool quoted = FALSE;
  bool lastQuoted = FALSE;
  QString line(256), sig, htmlStr;
  Kpgp* pgp = Kpgp::getKpgp();
  assert(pgp != NULL);
  assert(!aStr.isNull());

  if (pgp->setMessage(aStr))
  {
    if (pgp->isEncrypted())
    {      
      if(pgp->decrypt())
      {
	line.sprintf("<B>%s</B><BR>",
		     (const char*)i18n("Encrypted message"));
	htmlStr += line;
      }
      else
      {
	line.sprintf("<B>%s</B><BR>%s<BR><BR>",
		     (const char*)i18n("Cannot decrypt message:"),
		     (const char*)pgp->lastErrorMsg());
	htmlStr += line;
      }
    }
    pos = pgp->message().data();
  }
  else pos = aStr.data();

  // skip leading empty lines
  for (beg=pos; *pos && *pos<=' '; pos++)
  {
    if (*pos=='\n') beg = pos+1;
  }

  // check for PGP encryption/signing
  if (pgp->isSigned())
  {
    if (pgp->goodSignature()) sig = i18n("Message was signed by");
    else sig = i18n("Warning: Bad signature from");
    
    line.sprintf("<B>%s %s</B><BR>", sig.data(), 
		 pgp->signedBy().data());
    htmlStr += line;
  }

  pos = beg;
  while (1)
  {
    ch = *pos;
    if (ch=='\n' || ch=='\0')
    {
      *pos = '\0';
      line = strToHtml(beg,TRUE,TRUE);
      *pos = ch;
      if (quoted && !lastQuoted) line.prepend("<I>");
      else if (!quoted && lastQuoted) line.prepend("</I>");
      htmlStr += line + "<BR>\n";

      beg = pos+1;
      atStart = TRUE;
      lastQuoted = quoted;
      quoted = FALSE;
    }
    else if (ch > ' ' && atStart)
    {
      if (ch=='>' || ch==':' || ch=='|') quoted = TRUE;
      atStart = FALSE;
    }
    if (!ch) break;
    pos++;
  }

  mViewer->write(htmlStr);
}


//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum)
{
  QString iconName, href(255), label, comment;

  assert(aMsgPart!=NULL);

  label = aMsgPart->name();
  comment = aMsgPart->contentDescription();
  href.sprintf("part:%d", aPartNum+1);

  iconName = aMsgPart->iconName();
  if (iconName.left(11)=="unknown.xpm")
  {
    debug("determining magic type");
    aMsgPart->magicSetType();
    iconName = aMsgPart->iconName();
  }
  debug("href: "+href);
  mViewer->write("<TABLE><TR><TD><A HREF=\"" + href + "\"><IMG SRC=\"" + 
		 iconName + "\">" + label + "</A></TD></TR></TABLE>" +
		 comment + "<BR>");
}


//-----------------------------------------------------------------------------
const QString KMReaderWin::strToHtml(const QString aStr, bool aDecodeQP,
				     bool aPreserveBlanks) const
{
  QString htmlStr, qpstr, iStr;
  char ch, *pos, str[256];
  int i, i1, x, len;

  if (aDecodeQP) qpstr = KMMsgBase::decodeRFC1522String(aStr);
  else qpstr = aStr;

  for (pos=qpstr.data(),x=0; *pos; pos++,x++)
  {
    ch = *pos;
    if (aPreserveBlanks)
    {
      if (ch==' ' && pos[1]==' ')
      {
	htmlStr += " &nbsp;";
	for (pos++, x++; pos[1]==' '; pos++, x++)
	  htmlStr += "&nbsp;";
	continue;
      }
      else if (ch=='\t')
      {
	do
	{
	  htmlStr += "&nbsp;";
	  x++;
	}
	while((x&7) != 0);
      }
      // else aPreserveBlanks = FALSE;
    }
    if (ch=='<') htmlStr += "&lt;";
    else if (ch=='>') htmlStr += "&gt;";
    else if (ch=='\n') htmlStr += "<BR>";
    else if (ch=='&') htmlStr += "&amp;";
    else if ((ch=='h' && strncmp(pos,"http:",5)==0) ||
	     (ch=='f' && strncmp(pos,"ftp:",4)==0) ||
	     (ch=='m' && strncmp(pos,"mailto:",7)==0))
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
      htmlStr += "<A HREF=\"";
      htmlStr += str;
      htmlStr += "\">";
      htmlStr += str;
      htmlStr += "</A>";
    }
    else if (ch=='@')
    {
      char *startofstring = qpstr.data();
      char *startpos = pos;
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

      htmlStr.truncate(htmlStr.length() - i1 + 1);
      if (iStr.length()>3) 
	htmlStr += "<A HREF=\"mailto:" + iStr + "\">" + iStr + "</A>";
      else htmlStr += iStr;
      iStr = "";
    }

    else htmlStr += ch;
  }

  return htmlStr;
}


//-----------------------------------------------------------------------------
void KMReaderWin::printMsg(void)
{
  if (!mMsg) return;
  mViewer->print();
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const char* aUrl)
{
  if (!aUrl || !mMsg || strncmp(aUrl,"part:",5)) return -1;
  return (aUrl ? atoi(aUrl+5) : 0);
}


//-----------------------------------------------------------------------------
void KMReaderWin::resizeEvent(QResizeEvent *)
{
  mViewer->setGeometry(0, 0, width()-16, height()-16);
  mSbHorz->setGeometry(0, height()-16, width()-16, 16);
  mSbVert->setGeometry(width()-16, 0, 16, height()-16);

  mSbHorz->setSteps( 12, mViewer->width() - 12 );
  mSbVert->setSteps( 12, mViewer->height() - 12 );
}


//-----------------------------------------------------------------------------
void KMReaderWin::closeEvent(QCloseEvent *e)
{
  KMReaderWinInherited::closeEvent(e);
  writeConfig();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOn(const char* aUrl)
{
  int id;
  KMMessagePart msgPart;

  id = msgPartFromUrl(aUrl);
  if (id <= 0)
  {
    emit statusMsg(aUrl);
  }
  else
  {
    mMsg->bodyPart(id-1, &msgPart);
    emit statusMsg(i18n("Attachment: ") + msgPart.name());
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const char* aUrl, int aButton)
{
  int id;

  id = msgPartFromUrl(aUrl);
  if (id > 0)
  {
    // clicked onto an attachment
    mAtmCurrent = id-1;
    slotAtmOpen();
  }
  else emit urlClicked(aUrl, aButton);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const char* aUrl, const QPoint& aPos)
{
  KMMessagePart msgPart;
  int id;
  QPopupMenu *menu;

  id = msgPartFromUrl(aUrl);
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
  KMReaderWin* win = new KMReaderWin;
  assert(aMsgPart!=NULL);

  msg->fromString(aMsgPart->bodyDecoded());
  win->setMsg(msg);
  win->setAutoDelete(TRUE);
  win->show();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmView()
{
  QString str, pname;
  KMMessagePart msgPart;
  QMultiLineEdit* edt = new QMultiLineEdit;

  mMsg->bodyPart(mAtmCurrent, &msgPart);
  pname = msgPart.name();
  if (pname.isEmpty()) pname=msgPart.contentDescription();
  if (pname.isEmpty()) pname="unnamed";

  if (stricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  kbp->busy();
  str = msgPart.bodyDecoded();

  edt->setCaption(i18n("View Attachment: ") + pname);
  edt->insertLine(str);
  edt->setReadOnly(TRUE);
  edt->show();

  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  QString str, pname, cmd, fileName;
  KMMessagePart msgPart;
  char* tmpName;
  int old_umask;
  int c;

  mMsg->bodyPart(mAtmCurrent, &msgPart);

  if (stricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  pname = msgPart.name();
  if (pname.isEmpty()) pname="unnamed";
  tmpName = tempnam(NULL, NULL);
  if (!tmpName)
  {
    warning(i18n("Could not create temporary file"));
    return;
  }
  fileName = tmpName;
  free(tmpName);
  fileName += '-';
  fileName += pname;

  // remove quotes from the filename so that the shell does not get confused
  c = 0;
  while ((c = fileName.find('"', c)) >= 0)
    fileName.remove(c, 1);

  c = 0;
  while ((c = fileName.find('\'', c)) >= 0)
    fileName.remove(c, 1);

  kbp->busy();
  str = msgPart.bodyDecoded();
  old_umask = umask(077);
  if (!kStringToFile(str, fileName, TRUE))
    warning(i18n("Could not save temporary file %s"),
	    (const char*)fileName);
  umask(old_umask);
  kbp->idle();
  cmd = "kfmclient openURL \'";
  cmd += fileName;
  cmd += "\'";
  debug(cmd);
  system(cmd);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  KMMessagePart msgPart;
  QString fileName, str;
  fileName = QDir::currentDirPath();
  fileName.append("/");

  
  mMsg->bodyPart(mAtmCurrent, &msgPart);
  fileName.append(msgPart.name());
  debug (fileName.data());
  
  fileName = KFileDialog::getSaveFileName(fileName.data(), "*", this);
  if(fileName.isEmpty()) return;

  kbp->busy();
  str = msgPart.bodyDecoded();
  if (!kStringToFile(str, fileName, TRUE))
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
void KMReaderWin::slotScrollVert(int _y)
{
  mSbVert->setValue(_y);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollHorz(int _x)
{
  mSbHorz->setValue(_x);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollUp()
{
  mSbVert->setValue(mSbVert->value() - 10);	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  mSbVert->setValue(mSbVert->value() + 10);	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollPrior()
{
  mSbVert->setValue(mSbVert->value() - (int)(height()*0.8));	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollNext()
{
  mSbVert->setValue(mSbVert->value() + (int)(height()*0.8));	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentChanged()
{
  if (mViewer->docHeight() > mViewer->height())
    mSbVert->setRange(0, mViewer->docHeight() - 
		    mViewer->height());
  else
    mSbVert->setRange(0, 0);
  
  if (mViewer->docWidth() > mViewer->width())
    mSbHorz->setRange(0, mViewer->docWidth() - 
		    mViewer->width());
  else
    mSbHorz->setRange(0, 0);
}

QString KMReaderWin::copyText()
{
  QString temp;
  mViewer->getSelectedText(temp);
  return temp;
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentDone()
{
  // mSbVert->setValue(0);
}


//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"
