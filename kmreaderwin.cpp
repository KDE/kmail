// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include <qfiledlg.h>
#include <stdlib.h>

#ifndef KRN
#include "kmglobal.h"
#include "kmmainwin.h"
#endif

#include "kmimemagic.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmreaderwin.h"
#include "kfileio.h"
#include "kbusyptr.h"
#include "kmmsgpartdlg.h"
#include "kpgp.h"

#include <html.h>
#include <kapp.h>
#include <kconfig.h>
#include <mimelib/mimepp.h>
#include <qregexp.h>
#include <qstring.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <qbitmap.h>
#include <qcursor.h>

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

  mPicsDir = app->kdedir()+"/share/apps/kmail/pics/";
  mMsg = NULL;

  readConfig();
  initHtmlWidget();
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
}


//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = kapp->getConfig();

  config->setGroup("Reader");
  mAtmInline = config->readNumEntry("attach-inline", 100);
  mHeaderStyle = (HeaderStyle)config->readNumEntry("hdr-style", HdrFancy);
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig(bool aWithSync)
{
  KConfig *config = kapp->getConfig();

  config->setGroup("Reader");
  config->writeEntry("attach-inline", mAtmInline);
  config->writeEntry("hdr-style", (int)mHeaderStyle);

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
void KMReaderWin::setHeaderStyle(KMReaderWin::HeaderStyle aHeaderStyle)
{
  mHeaderStyle = aHeaderStyle;
  update();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setInlineAttach(int aAtmInline)
{
  mAtmInline = aAtmInline;
  update();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMsg(KMMessage* aMsg)
{
  if (!parent())
  {
    // do some additional work for a standalone window.
    
  }

  mMsg = aMsg;

  if (!parent())
  {
    // do some additional work for a standalone window.
    
  }

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
  KMMessagePart msgPart;
  int i, numParts;
  QString type, subtype, str;

  assert(mMsg!=NULL);

  mViewer->begin(mPicsDir);
  mViewer->write("<HTML><BODY>");

  writeMsgHeader();

  numParts = mMsg->numBodyParts();
  if (numParts > 0)
  {
    for (i=0; i<numParts; i++)
    {
      mMsg->bodyPart(i, &msgPart);
      type = msgPart.typeStr();
      subtype = msgPart.subtypeStr();
      if (stricmp(type, "text")==0)
      {
	str = msgPart.bodyDecoded();
	if (str.size() > 100 && i>0) writePartIcon(&msgPart, i);
	else if (stricmp(subtype, "html")==0)
	{
	  if (i>0) mViewer->write("<BR><HR><BR>");
	  mViewer->write(str);
	}
	else
	{
	  if (i>0) mViewer->write("<BR><HR><BR>");
	  writeBodyStr(str);
	}
      }
      else
      {
	writePartIcon(&msgPart, i);
      }
    }
  }
  else
  {
    writeBodyStr(mMsg->bodyDecoded());
  }

  mViewer->write("<BR></BODY></HTML>");
  mViewer->end();
  mViewer->parse();
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeMsgHeader(void)
{
  switch (mHeaderStyle)
  {
  case HdrBrief:
    mViewer->write("<B>" + strToHtml(mMsg->subject()) + "</B> (" +
		   KMMessage::emailAddrAsAnchor(mMsg->from()) + ", " +
		   strToHtml(mMsg->dateShortStr()) + ")<BR><BR>");
    break;

  case HdrStandard:
    mViewer->write("<FONT SIZE=+1><B>" +
		   strToHtml(mMsg->subject()) + "</B></FONT><BR>");
    mViewer->write(nls->translate("From: ") +
		   KMMessage::emailAddrAsAnchor(mMsg->from()) + "<BR>");
    mViewer->write(nls->translate("To: ") +
                   KMMessage::emailAddrAsAnchor(mMsg->to()) + "<BR><BR>");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(nls->translate("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR><BR>");
#endif

    break;

  case HdrFancy:
    mViewer->write(QString("<TABLE><TR><TD><IMG SRC=") + mPicsDir +
		   "kdelogo.xpm></TD><TD HSPACE=50><B><FONT SIZE=+1>");
    mViewer->write(strToHtml(mMsg->subject()) + "</FONT><BR>");
    mViewer->write(nls->translate("From: ")+
		   KMMessage::emailAddrAsAnchor(mMsg->from()) + "<BR>");
    mViewer->write(nls->translate("To: ") +
		   KMMessage::emailAddrAsAnchor(mMsg->to()) + "<BR>");
    if (!mMsg->cc().isEmpty())
      mViewer->write(nls->translate("Cc: ")+
		     KMMessage::emailAddrAsAnchor(mMsg->cc()) + "<BR>");
    mViewer->write(nls->translate("Date: ") +
		   strToHtml(mMsg->dateStr()) + "<BR>");
#ifdef KRN
    if (!mMsg->references().isEmpty())
        mViewer->write(nls->translate("References: ") +
                       KMMessage::refsAsAnchor(mMsg->references()) + "<BR><BR>");
#endif
    mViewer->write("</B></TD></TR></TABLE><BR>");
    break;

  case HdrLong:
    emit statusMsg("`long' header style not yet implemented.");
    break;

  case HdrAll:
    emit statusMsg("`all' header style not yet implemented.");
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
  QString line, sig;
  Kpgp* pgp = Kpgp::getKpgp();

  assert(!aStr.isNull());
  assert(pgp != NULL);

#ifdef PGP
  if (pgp->setMessage(aStr))
  {
    if (pgp->isEncrypted())
    {
      if(!pgp->decrypt()) pgp->decrypt(pgp->askForPass());
    }
    pos = pgp->getMessage().data();
  }
  else pos = aStr.data();
#else
  pos = aStr.data();
#endif

  // skip leading empty lines
  for (beg=pos; *pos && *pos<=' '; pos++)
  {
    if (*pos=='\n') beg = pos+1;
  }

  if (pgp->isSigned())
  {
#ifdef PGP
    if (pgp->goodSignature()) sig = nls->translate("Message was signed by");
    else sig = nls->translate("Warning: Bad signature from");

    line.sprintf("<B>%s %s</B><BR><BR>", sig, (const char*)pgp->signedBy());
    mViewer->write(line);
#endif
  }

  pos = beg;
  for (beg=pos; *pos; pos++)
  {
    ch = *pos;
    if (ch=='\n')
    {
      *pos = '\0';
      line = strToHtml(beg);
      *pos='\n';
      if (quoted && !lastQuoted) line.prepend("<I>");
      else if (!quoted && lastQuoted) line.prepend("</I>");

      mViewer->write(line + "<BR>");

      beg = pos+1;
      atStart = TRUE;
      lastQuoted = quoted;
      quoted = FALSE;
      continue;
    }
    if (ch > ' ' && atStart)
    {
      if (ch=='>' || ch==':' || ch=='|') quoted = TRUE;
      atStart = FALSE;
    }
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum)
{
  QString iconName, href, label, comment;

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
const QString KMReaderWin::strToHtml(const QString aStr, bool aDecodeQP) const
{
  QString htmlStr, qpstr;
  char ch, *pos, str[256];
  int i;

  if (aDecodeQP) qpstr = KMMsgBase::decodeQuotedPrintableString(aStr);
  else qpstr = aStr;

  for (pos=qpstr.data(); *pos; pos++)
  {
    ch = *pos;
    if (ch=='<') htmlStr += "&lt;";
    else if (ch=='>') htmlStr += "&gt;";
    else if (ch=='&') htmlStr += "&amp;";
    else if ((ch=='h' && strncmp(pos,"http:",5)==0) ||
	     (ch=='f' && strncmp(pos,"ftp:",4)==0))
    {
      for (i=0; *pos && *pos>' ' && i<255; i++, pos++)
	str[i] = *pos;
      str[i] = '\0';
      pos--;
      htmlStr += "<A HREF=\"";
      htmlStr += str;
      htmlStr += "\">";
      htmlStr += str;
      htmlStr += "</A>";
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
  mViewer->setGeometry(0, 0, width()-16, height());
  mSbHorz->setGeometry(0, height()-16, width()-16, 16);
  mSbVert->setGeometry(width()-16, 0, 16, height());
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
  if (id < 0)
  {
    emit statusMsg(aUrl);
  }
  else
  {
    mMsg->bodyPart(id, &msgPart);
    emit statusMsg(msgPart.name());
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
    slotAtmSave();
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
  if (id <= 0) emit popupMenu(aPos);
  else
  {
    mAtmCurrent = id-1;
    menu = new QPopupMenu();
    menu->insertItem(nls->translate("Open..."), this, SLOT(slotAtmOpen()));
    menu->insertItem(nls->translate("Save as..."), this, SLOT(slotAtmSave()));
    //menu->insertItem(nls->translate("Print..."), this, SLOT(slotAtmPrint()));
    menu->insertItem(nls->translate("Properties..."), this,
		     SLOT(slotAtmProperties()));
    menu->popup(aPos,0);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  KMMessagePart msgPart;
  mMsg->bodyPart(mAtmCurrent, &msgPart);
  
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  KMMessagePart msgPart;
  QString fileName, str;

  mMsg->bodyPart(mAtmCurrent, &msgPart);
  
  fileName = msgPart.name();
  fileName = QFileDialog::getSaveFileName(NULL, "*", this);
  if(fileName.isEmpty()) return;

  kbp->busy();
  str = msgPart.bodyDecoded();
  if (!kStringToFile(str, fileName, TRUE))
    warning(nls->translate("Could not save file"));
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
  KMMsgPartDlg  dlg;

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
