// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include "kmglobal.h"
#include "kmimemagic.h"
#include "kmmainwin.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmreaderwin.h"

#include <html.h>
#include <kapp.h>
#include <kconfig.h>
#include <mimelib/mimepp.h>
#include <qaccel.h>
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


//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent, const char *aName, int aFlags)
  :KMReaderWinInherited(aParent, aName, aFlags)
{
  initMetaObject();

  mPicsDir = app->kdedir()+"/share/apps/kmail/pics/";
  mMsg = NULL;

  readConfig();
  initHtmlWidget();

#ifdef BROKEN
  QAccel *accel = new QAccel(this);
  int UP =200;
  int DOWN = 201;
  accel->insertItem(Key_Up,UP);
  accel->insertItem(Key_Down,DOWN);
  accel->connectItem(UP, this, SLOT(slotScrollUp()));
  accel->connectItem(DOWN,this,SLOT(slotScrollDown()));

  if(currentMessage)
    parseMessage(currentMessage);
  else
   clearCanvas();
#endif //BROKEN
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
  mViewer->setDefaultBGColor(app->activeTextColor);
  mViewer->setURLCursor(handCursor);

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
  KMMessagePart msgPart;
  int i, numParts;
  QString type, subtype;

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
	mViewer->write("<HR>");
	if (stricmp(subtype, "html")==0)
	  mViewer->write(msgPart.bodyDecoded());
	else
	  writeBodyStr(msgPart.bodyDecoded());
      }
      else
      {
	writePartIcon(&msgPart, i+1);
      }
    }
  }
  else
  {
    writeBodyStr(mMsg->body());
  }

  mViewer->write("</BODY></HTML>");
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
    break;

  case HdrLong:
    debug("long header style not yet implemented.");
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
    mViewer->write("</B></TD></TR></TABLE><BR>");
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
  QString line;

  assert(!aStr.isNull());

  // skip leading empty lines
  pos = aStr.data();
  for (beg=pos; *pos && *pos<=' '; pos++)
  {
    if (*pos=='\n') beg = pos+1;
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
  href.sprintf("part:%d", aPartNum);

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
const QString KMReaderWin::strToHtml(const QString aStr) const
{
  QString htmlStr;
  char ch, *pos;

  for (pos=aStr.data(); *pos; pos++)
  {
    ch = *pos;
    if (ch=='<') htmlStr += "&lt;";
    else if (ch=='>') htmlStr += "&gt;";
    else if (ch=='&') htmlStr += "&am;";
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

  if (!mMsg) return;
  id = aUrl ? atoi(aUrl) : 0;

  debug("slotUrlOn(%s) called", aUrl);

  if (id > 0)
  {
    mMsg->bodyPart(id-1, &msgPart);
    emit statusMsg(msgPart.name());
  }
  else emit statusMsg(aUrl);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const char* aUrl, int aButton)
{
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const char* aUrl, const QPoint& aPos)
{
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
  int i = mSbVert->value();
  i = i - 7;
  mSbVert->setValue(i);	
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  int i = mSbVert->value();
  i = i + 7;
  mSbVert->setValue(i);
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


//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentDone()
{
  // mSbVert->setValue(0);
}








//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#ifdef BROKEN

void KMReaderWin::parseMessage(KMMessage *message)
{
  QString strTemp;
  QString str1Temp;
  QString subjStr;
  QString text;
  QString header;
  QString dateStr;
  QString fromStr;
  QString toStr;
  QString ccStr;
  int i, numParts=0;
  KMMessagePart msgPart;

  currentMessage = message; // To make sure currentMessage is set.


  dateStr = "Date: " + message->dateStr() + "<BR>";
  fromStr = "From: " + KMMessage::emailAddrAsAnchor(message->from()) + "<BR>";

  ccStr = message->cc();
  if(!ccStr.isEmpty())
    ccStr= "Cc: " + KMMessage::emailAddrAsAnchor(message->cc()) + "<BR>";
			 
  subjStr = "<FONT SIZE=+1> Subject: " + message->subject() + "</FONT><P>";
  toStr = "To: " + message->to() + "<BR>";

  // Init mViewer
  mViewer->begin(picsDir);

  // header
  mViewer->write("<TABLE><TR><TD><IMG SRC=\"" + 
		       picsDir +"/kdelogo.xpm\"></TD><TD HSPACE=50><B>");
  mViewer->write(subjStr);
  mViewer->write(fromStr);
  mViewer->write(toStr);
  mViewer->write(ccStr);
  mViewer->write(dateStr);
  mViewer->write("</B></TD></TR></TABLE><br><br>");	

  numParts = message->numBodyParts();
  if (numParts <= 0)
  {
    text = message->body().stripWhiteSpace();
    text += "\n";
  }
  else
  {
    for(i=0; i<numParts; i++)
    {
      debug("processing body part %d of %d", i, numParts);
      message->bodyPart(i, &msgPart);
      text += parseBodyPart(&msgPart, i);
    }			
  }

  // Convert text to html
  text.replace(QRegExp("\n"),"<BR>");
  text.replace(QRegExp("\\x20",FALSE,FALSE),"&nbsp"); // SP
  
    	
  mViewer->write("<HTML><HEAD><TITLE> </TITLE></HEAD>");
  mViewer->write("<BODY BGCOLOR=WHITE>");


  // Okay! Let's write it to the canvas
  mViewer->write(text);
  mViewer->write("<BR></BODY></HTML>");
  mViewer->end();
  mViewer->parse();
}


QString KMReaderWin::parseBodyPart(KMMessagePart *p, int pnumber)
{
  QString text;
  QString type;
  QString subType;

  QString pnumstring;
  QString temp;
  QString comment;
  int pos;

  assert(p != NULL);

  debug("parsing part #%d: ``%s''", pnumber, (const char*)p->name());
  comment = p->name();
  pnumstring.sprintf("file:/%i",pnumber);
  text = p->bodyDecoded(); // Decode bodyPart

  // ************* MimeMagic stuff ****************// 
  // This has to be improved vastly. For gz files (e.g) mimemagic still
  // says it is an octet-stream and not a gzip file

  KMimeMagicResult *result = new KMimeMagicResult();
  result = magic->findBufferType(text,text.length()-1); // Removed -1	
  if(result->getAccuracy() <= 50)
  {
    debug("  the accuracy is <= 50 ... should look at filename ending\n");
  }
  temp =  result->getContent(); // Determine Content Type
  pos = temp.find("/",0,0);
  type = temp.copy();
  subType = temp.copy();
  type.truncate(pos);
  subType = subType.remove(0,pos+1); 
  debug("  type: %s\n  subtype: %s", (const char*)type, (const char*)subType);
  // ************* MimeMagic stuff end *****************//

  QString fileName;
  if(type == "text" && pnumber ==0) // If first bodyPart && type=="text" 
  // Then we do not want it do be displayed as an icon.
    {cout << "is text & 0\n";
    text += "<br><hr><br>";
    return text;
    }
  if(isInline() == false) // If we do not want 
    //the attachments to be displayed inline
    {cout << "is Inline\n";
    return bodyPartIcon(type, subType, pnumstring, comment);
    }

  if(type == "text" && pnumber != 0)  // If content type is text.
    {if(text.length()/80 > MAX_LINES) // Check for max_lines.
      {temp.sprintf("The text attachment has more than %i lines.\n",
		    MAX_LINES); 
       temp += "Do you wish the attachment to be displayed inline?";
       if(KMsgBox::yesNo(0,"KMail Message",temp) == 1)
	 return bodyPartIcon(type, subType, pnumstring, comment);
      }
    text += "<br><hr><br>";
    return text;
    }
  
  else if(type == "message" && pnumber != 0)// If content type is message just display the text.
    {text += "<br><hr><br>";
    return text;
    }

  cout << "Neither text nor message\n";
  return bodyPartIcon(type, subType, pnumstring, comment);
    
}

QString KMReaderWin::bodyPartIcon(QString type, QString subType,
				QString pnumstring, QString comment)
{
  QString text, icon, fileName, path;
  QDir dir;
  
  path = KApplication::kdedir() + "/share/mimelnk/";
  fileName = path + type + "/" + subType + ".kdelnk";

  if (!dir.exists(fileName) && type == "text")
    fileName = path + type + "/plain.kdelnk";

  if (dir.exists(fileName))
  {
    KConfig config(fileName);
    config.setGroup("KDE Desktop Entry");
    icon = config.readEntry("Icon");
    if(icon.isEmpty()) // If no icon specified.
      icon = KApplication::kdedir()+ "/share/icons/unknown.xpm";
    else icon.prepend(KApplication::kdedir()+ "/share/icons/"); // take it

  }
  else
  {
    icon = KApplication::kdedir() + "/share/icons/unknown.xpm";
  }

  text = "<TABLE><TR><TD><A HREF=\"" + pnumstring + "\"><IMG SRC=" + 
         icon + ">" + comment + "</A></TD></TR></TABLE>" + "<BR><HR><BR>";

  return text;

}


QString KMReaderWin::parseEAddress(QString old)
{
  int pos;
  if((pos = old.find("<",0,0)) == -1)
    {old = "<A HREF=\"mailto:" + old + "\">" + old +"</A>";
    cout << old << "\n";
    return old;
    }

}


bool KMReaderWin::saveMail()
{
  QString fileName;
  QString text;
  QString err_str;

  // QFileDialog can't take subject (e.g) as default savefilename yet.
  fileName = QFileDialog::getSaveFileName(); 
  if(fileName.isEmpty())
    return false;

  QFile *file = new QFile(fileName);
  if(file->exists()) 
    {if(!KMsgBox::yesNo(0,"Save Mail",
			"File already exists!\nOverwrite it?"))
      if(saveMail() == false)
	return false;
    else
      return false;
    }

  if(!file->open(IO_ReadWrite | IO_Truncate))
    {err_str = "Error opening saving File!\n";
    err_str.append(strerror(errno)) ;
    KMsgBox::message(0,"Save Body Part",err_str);
    if(saveMail() == false)
      return false;
    }
  text = currentMessage->asString();
  if(file->writeBlock(text,text.length()) == -1)
    {KMsgBox::message(0,"QFile::writeBlock()",
		      "Serious error occured! Returning...");
    return false;
    }

  file->close();
  return true;


}

void KMReaderWin::slotOpenAtmnt()
{
  /*  if(!currentMessage)
    return;
  printf("CurrentAtmnt :%i\n",currentAtmnt);*/
  ((KMReaderWin*)parentWidget())->toDo();
}

bool KMReaderWin::slotSaveAtmnt()
{
  QString fileName;
  QString text;
  QString err_str;

  KMMessagePart *p = new KMMessagePart();
  currentMessage->bodyPart(currentAtmnt,p);
  debug("KMReaderWin::slotSaveAtmnt(): before save-decoding");
  text = p->bodyDecoded();
  debug("KMReaderWin::slotSaveAtmnt(): after save-decoding");
  fileName = p->name();

  // QFileDialog can't take p->name() as default savefilename yet.
  fileName = QFileDialog::getSaveFileName(NULL, "*", NULL, (const char*)fileName);
  if(fileName.isEmpty()) return false;

  QFile *file = new QFile(fileName);
  if(file->exists()) 
    {if(!KMsgBox::yesNo(0,"Save Body Part",
			"File already exists!\nOverwrite it?"))
      if(slotSaveAtmnt() == false)
	return false;
    else
      return false;
    }

  if(!file->open(IO_ReadWrite | IO_Truncate))
    {err_str = "Error opening saving File!\n";
    err_str.append(strerror(errno)) ;
    KMsgBox::message(0,"Save Body Part",err_str);
    if(slotSaveAtmnt() == false)
      return false;
    }

  if(file->writeBlock(text,text.length()) == -1)
    {KMsgBox::message(0,"QFile::writeBlock()",
		      "Serious error occured! Returning...");
    return false;
    }

  file->close();
  return true;
}

bool KMReaderWin::slotPrintAtmnt()
{
  QString text;
  QString err_str;

  KMMessagePart *p = new KMMessagePart();
  currentMessage->bodyPart(currentAtmnt,p);
  printf("before print decoding\n");
  text = p->bodyDecoded();
  printf("after print decoding\n");

  // Some work to do.

  return true;
}

void KMReaderWin::openURL(const char *url, int)
{
  // Once I have autoscanning of urls implemented which is 
  // a pain cause I just hate parsing strings, selecting the url
  // will call this function which will invoke kfm or the composer

  printf("URL selected\n");
  QString fullURL;
  fullURL = url;
  cout << fullURL << "\n";
  
  if (fullURL.find("http:") >= 0)
    {QString cmd = "kfmclient exec ";
    cmd += fullURL;
    cmd += " Open";
    system(cmd);
    }
  else if (fullURL.find("ftp:") >= 0)
    {
      QString cmd = "kfmclient exec ";
      cmd += fullURL;
      cmd += " Open";
      system(cmd);
    }
  else if (fullURL.find("mailto:") >= 0)
  {
    KMMessage* msg;
    fullURL.remove(0,7);
    msg->initHeader();
    msg->setTo(fullURL);
    KMComposeWin *w = new KMComposeWin(msg);
    w->show();
  }

}
void KMReaderWin::popupHeaderMenu(const char *_url, const QPoint &cords)
{
  QString url = _url;
  if(!url.isEmpty() && (url.find("@",0,0) != -1))
    {QPopupMenu *p = new QPopupMenu();
    p->insertItem("Add to Addressbook");
    p->insertItem("Properties");
    p->popup(cords,0);
    }
  
}

void KMReaderWin::popupMenu(const char *_url, const QPoint &cords)
{
  QString temp=_url;
  int number;
  
  if(temp.isEmpty())
    {QPopupMenu *p = new QPopupMenu();
    p->insertItem("Reply to Sender",this,SLOT(replyMessage()));
    p->insertItem("Reply to All Recipients",this,SLOT(replyAll()));		   p->insertItem("Forward Message",this,SLOT(forwardMessage()));
    p->insertSeparator();
    QPopupMenu *folderMenu = new QPopupMenu(); 
    folderMenu->insertItem("Inbox");
    folderMenu->insertItem("Sent messages");
    folderMenu->insertItem("Trash");
    p->insertItem("Send Message to Folder",folderMenu);
    p->insertItem("Delete Message",this,SLOT(deleteMessage()));
    p->insertItem("Save Message",this,SLOT(saveMail()));		
    p->insertItem("Print Message",this,SLOT(printMail()));
    p->popup(cords,0);}
  
  else
    {if(temp.find("@",0,0) != -1)// Check if mailto url
      {popupHeaderMenu(temp,cords);
      return;
      }
    temp.replace(QRegExp("file:/"),"");
    cout << temp << "\n";
    number = temp.toUInt();
    printf("BodyPart : %i\n",number);
    currentAtmnt = number;
    QPopupMenu *p = new QPopupMenu();
    p->insertItem("Open...",this,SLOT(slotOpenAtmnt()));
    p->insertItem("Print...",this,SLOT(slotPrintAtmnt()));
    p->insertItem("Save as...",this,SLOT(slotSaveAtmnt()));
    p->popup(cords,0);}
  
} 

void KMReaderWin::copy()
{
  mViewer->getSelectedText(selectedText);
}

void KMReaderWin::markAll()
{
}

void KMReaderWin::viewSource()
{
  QString text;
  KMProperties *p = new KMProperties(0,0,currentMessage);
  p->show();
  p->resize(p->size());
}


bool KMReaderWin::isInline()
{
  if(showInline == true)
    return true;
  else
      return false;
}


void KMReaderWin::setInline(bool _inline)
{
  showInline=_inline;
  updateDisplay();
}

/***************************************************************************/



KMReaderWin::KMReaderWin(QWidget *, const char *, int msgno = 0,KMFolder *f =0)
	:KTopLevelWidget(NULL)
{

  tempFolder = new KMFolder();
  tempFolder = f;
  setCaption("KMail Reader");

  parseConfiguration();

  newView = new KMReaderWin(this,NULL, msgno,f);
  setView(newView);

  setupMenuBar();

  setupToolBar();

  if(!showToolBar)
	enableToolBar(KToolBar::Hide);
  resize(480, 510);
}

// ******************** Public slots ********************


void KMReaderWin::parseConfiguration()
{
  KConfig *config;
  QString o;
  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  o = config->readEntry("Reader ShowToolBar");
  if((!o.isEmpty() && o.find("no",0,false)) == 0)
	showToolBar = 0;
  else
	showToolBar = 1;
}

void KMReaderWin::doDeleteMessage()
{
}


// ***************** Private slots ********************


void KMReaderWin::invokeHelp()
{

  kapp->invokeHTMLHelp("","");

}

void KMReaderWin::toDo()
{
  KMsgBox::message(this,"Ouch",
		   "Not yet implemented!\n"
		   "We are sorry for the inconvenience :-)",1);
}

void KMReaderWin::newComposer()
{
  KMComposeWin *k = new KMComposeWin();
  k->show();
  k->resize(k->size());
}

void KMReaderWin::newReader()
{
/*	KMMainWin *w = new KMMainWin();
	w->show();
	w->resize(w->size());*/
}

void KMReaderWin::about()
{
  KMsgBox::message(this,"About",
		   "kmail [ALPHA]\n\n"
		   "Yat-Nam Lo <lynx@topaz.hknet.com>\n"
		   "Stephan Meyer <Stephan.Meyer@munich.netsurf.de>\n"
		   "Stefan Taferner <taferner@alpin.or.at>\n"
		   "Markus Wübben <markus.wuebben@kde.org>\n\n"
		   "This program is covered by the GPL.",1);
}

void KMReaderWin::toggleToolBar()
{
  enableToolBar(KToolBar::Toggle);
}

void KMReaderWin::abort()
{
  close();
}


// **************** Protected ************************



KMProperties::KMProperties(QWidget *parent=0, const char *name=0, KMMessage *cM=0)
  : QDialog(parent, name)
{
  QString text = cM->asString();
  setMaximumSize(410,472);
  setMinimumSize(410,472);
  
  tabDialog = new QTabDialog(this,"main",FALSE,0);
  tabDialog->move(0,0);

  topLevel = new KMGeneral(tabDialog,"kfs",cM);
  tabDialog->addTab(topLevel,"General");

  sourceWidget = new KMSource(tabDialog,"Source", text);
  tabDialog->addTab(sourceWidget,"Source");

  connect(tabDialog,SIGNAL(applyButtonPressed()),qApp,SLOT(quit()));
}
  
KMProperties::~KMProperties()
{
  delete topLevel;
  delete sourceWidget;
  delete tabDialog;
}

KMGeneral::KMGeneral(QWidget *parent=0, const char *name=0, KMMessage *cM=0)
  : QDialog(parent, name)
{
  setMinimumSize(400,370);
  setMaximumSize(400,370);
  tempMes = new KMMessage();
  tempMes =cM;
}

void KMGeneral::paintEvent(QPaintEvent *)
{
  QPainter p;
  QString temp;
  p.begin(this);
  QPoint point(20,30);
  temp = KApplication::kdedir() + "/share/apps/kmail/send.xpm";	
  QPixmap pix(temp);
  p.drawPixmap(point,pix);
  p.setPen(black);
  temp = "Subject: " + tempMes->subject();
  p.drawText(60, 30, temp);
  temp = "From: " + tempMes->from();
  p.drawText(60, 60, temp);
  p.drawLine(10,80,380,80);
  temp = tempMes->asString();
  temp.sprintf("Size: %i kb",temp.length()/1024);
  p.drawText(20,100,temp);
  p.drawText(20,130,"Folder:");
  p.drawLine(10,150,380,150);
  temp.sprintf("Attachments: %i", tempMes->numBodyParts());
  p.drawText(20,170,temp);
  p.drawText(20,200,"Format:");
  p.drawLine(10,220,380,220);
  temp = "Sent: " + tempMes->dateStr();
  p.drawText(20,240,temp);
  p.drawText(20,270,"Recieved on:");
  p.end();
  
}

KMSource::KMSource(QWidget *parent=0, const char *name=0,QString text=0)
  : QDialog(parent, name)
{
  edit = new QMultiLineEdit(this);
  edit->resize(360,340);
  edit->move(20,20);
  edit->setText(text);
  edit->setReadOnly(TRUE);
}

#endif //BROKEN


//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"
