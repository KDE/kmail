// kmreaderwin.cpp

#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmreaderwin.h"
#include "kmreaderwin.moc"
#include "kmimemagic.h"
#include <kiconloader.h>

KMReaderView::KMReaderView(QWidget *parent =0, const char *name = 0, int msgno = 0,KMFolder *f = 0)
	:QWidget(parent,name)
{
  printf("Entering view:  msgno = %i \n",msgno);

  kdeDir = getenv("KDEDIR");
  if(!kdeDir)
       	{KMsgBox::message(0,"Ouuch","$KDEDIR not set.\nPlease do so");
       	qApp->quit();}

 picsDir.append(kdeDir);
 picsDir +="/share/apps/kmail/pics"; 

 currentFolder = new KMFolder();
 currentFolder = f;

 currentMessage = new KMMessage();
 currentIndex = msgno;

 if(f !=0)
    currentMessage = f->getMsg(msgno);
 else
    currentMessage= NULL;

	// Let's initialize the HTMLWidget

  messageCanvas = new KHTMLWidget(this,0,picsDir);
  messageCanvas->setURLCursor(upArrowCursor);
  messageCanvas->resize(parent->width()-16,parent->height()-110); //16
  connect(messageCanvas,SIGNAL(URLSelected(const char *,int)),this,
	  SLOT(openURL(const char *,int)));
  connect(messageCanvas,SIGNAL(popupMenu(const char *, const QPoint &)), 
	  SLOT(popupMenu(const char *, const QPoint &)));
  vert = new QScrollBar( 0, 110, 12, messageCanvas->height()-110, 0,
                        QScrollBar::Vertical, this, "vert" );
  horz = new QScrollBar( 0, 0, 24, messageCanvas->width()-16, 0,
                        QScrollBar::Horizontal, this, "horz" );	
  connect( messageCanvas, SIGNAL( scrollVert( int ) ), 
	SLOT( slotScrollVert(int)));
  connect( messageCanvas, SIGNAL( scrollHorz( int ) ), 
	   SLOT( slotScrollHorz(int)));
  connect( vert, SIGNAL(valueChanged(int)), messageCanvas, 
	   SLOT(slotScrollVert(int)));
  connect( horz, SIGNAL(valueChanged(int)), messageCanvas, 
	   SLOT(slotScrollHorz(int)));
  connect( messageCanvas, SIGNAL( documentChanged() ), 
	   SLOT( slotDocumentChanged() ) );
  connect( messageCanvas, SIGNAL( documentDone() ), 
	   SLOT( slotDocumentDone() ) );	
		
	// Puh, okay this is done

  QAccel *accel = new QAccel( this );       
  accel->connectItem( accel->insertItem(Key_Up),this,SLOT(slotScrollLeRi()));
  accel->connectItem( accel->insertItem(Key_Down),this,SLOT(slotScrollUpDo()));

  if(currentMessage)
    parseMessage(currentMessage);
  else
   clearCanvas();

  initKMimeMagic();

  parseConfiguration();
}

void  KMReaderView::parseConfiguration()
{
  QString o;
  KConfig *config = new KConfig();
  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  MAX_LINES = config->readNumEntry("Lines");
}




// *********************** Public slots *******************

void KMReaderView::clearCanvas()
{
	// Produce a white canvas
  messageCanvas->begin(picsDir);
  messageCanvas->write("<HTML><BODY BGCOLOR=WHITE></BODY></HTML>");
  messageCanvas->end();
  messageCanvas->parse();
}

void KMReaderView::updateDisplay()
{
  if(currentMessage != 0)
    {clearCanvas();
    parseMessage(currentMessage);
    }
}


// ********************** Protected **********************

void KMReaderView::resizeEvent(QResizeEvent *)
{
  messageCanvas->setGeometry(0,0,this->width()-16,this->height()); //16
  horz->setGeometry(0,height()-16,width()-16,16);
  vert->setGeometry(width()-16,0,16,height());
}


// ******************* Private slots ********************

void KMReaderView::parseMessage(KMMessage *message)
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
  long length;
  int multi=0;
  int pos=0;
  int numParts=0;

  currentMessage = message; // To make sure currentMessage is set.


  dateStr.sprintf("Date: %s<br>",message->dateStr());

  strTemp.sprintf("%s",message->from());
  if((pos=strTemp.find("<",0,0)) != -1)
    {strTemp.remove(0,pos+1);
    strTemp.replace(QRegExp(">",0,0),"");
    }
  fromStr.sprintf("From: <A HREF=\"mailto:");
  fromStr.append(strTemp + "\">");
  if(pos != -1)
    {strTemp.sprintf("%s",message->from());
    strTemp.truncate(pos);
    }
  strTemp = strTemp.stripWhiteSpace();
  fromStr.append(strTemp + "</A>"+"<br>");

  ccStr.sprintf("%s",message->cc());
  if(ccStr.isEmpty())
    ccStr = "";
  else
    ccStr.sprintf("Cc: %s",message->cc());
			 
  subjStr.sprintf("Subject: %s<br><P>",message->subject());	
  toStr.sprintf("To: %s<br>", message->to());

  // Init messageCanvas
  messageCanvas->begin(picsDir);

  // header
  messageCanvas->write("<TABLE><TR><TD><IMG SRC=\"" + 
		       picsDir +"/kdelogo.xpm\"></TD><TD HSPACE=50><B>");
  messageCanvas->write(subjStr);
  messageCanvas->write(fromStr);
  messageCanvas->write(toStr);
  messageCanvas->write(ccStr);
  messageCanvas->write(dateStr);
  messageCanvas->write("</B></TD></TR></TABLE><br><br>");	

  // Prepare text
  if((numParts = message->numBodyParts()) == 0)
    {text = message->body(&length);
    text.truncate(length);
    }
  //text = scanURL(text);}
  else
    {KMMessagePart *part = new KMMessagePart();
    for(multi=0;multi < numParts;multi++)
      {printf("in body part loop\n");
      message->bodyPart(multi,part);
      text += parseBodyPart(part,multi);
      delete part;
      part= new KMMessagePart();
      }
    }			

    // Convert text to html

  text.replace(QRegExp("\n"),"<BR>");
  text.replace(QRegExp("\\x20",FALSE,FALSE),"&nbsp"); // SP
    	
  messageCanvas->write("<HTML><HEAD><TITLE> </TITLE></HEAD>");
  messageCanvas->write("<BODY BGCOLOR=WHITE>");
  // Okay! Let's write it to the canvas
  messageCanvas->write(text);
  messageCanvas->write("</BODY></HTML>");
  messageCanvas->end();
  messageCanvas->parse();
}


QString KMReaderView::parseBodyPart(KMMessagePart *p, int pnumber)
{
  QString text;
  QString type;
  QString subType;

  QString pnumstring;
  QString temp;
  int pos;

  pnumstring.sprintf("file:/%i",pnumber);

  KMimeMagicResult *result = new KMimeMagicResult();
  text = decodeString(p,p->cteStr());
  result = magic->findBufferType(text,text.length()-1);	
  temp =  result->getContent(); // Determine Content Type
  pos = temp.find("/",0,0);
  cout << "pos:" << pos << endl << "temp: "<< temp << endl;
  type = temp.copy();
  subType = temp.copy();
  type.truncate(pos);
  subType = subType.remove(0,pos+1); 
  cout << "Type:" << type << endl << "SubType:" << subType <<endl;

  printf("Debug :%i\n",showInline);
  if(showInline == false) // If we do not want 
                          //the attachments to be displayed inline
    {QString icon;
     QFile *file = new QFile(KApplication::kdedir()+"/share/mimelnk/" 
			     + type + subType); // Search for mimetype.
     if(file->open(IO_ReadOnly)) // if mimetype exists                        
        {QTextStream pstream(file);         
	KConfig config(&pstream);
	config.setGroup("KDE Desktop Entry");
	icon = config.readEntry("Icon");
	if(icon.isEmpty()) // If no icon specified.
	  icon = KApplication::kdedir()+ "/share/icons/unknown.xpm";
	else
	  icon.prepend(KApplication::kdedir()+ "/share/icons/"); // take it
	QString comment = config.readEntry("Comment");
	file->close();
	text = "<A HREF=\"" + pnumstring +"\"><IMG SRC=" + icon 
	  + ">" + comment + "</A>";
	text += "<br><hr><br>";
	return text;
	}
     else
       {icon = KApplication::kdedir()+ "/share/icons/unknown.xpm";
	printf("Not a registered mimetype\n");
	text = "<A HREF=\"" + pnumstring +"\"><IMG SRC=" + icon 
	  + ">Unknown content</A>";
	text += "<br><hr><br>";
	return text;	
       }
    }

  if(type == "text")  // If content type is text.
    {cout << "isText\n";
    if(text.length()/80 > MAX_LINES) // Check for max_lines.
      {temp.sprintf("The text attachment has more than %i lines.\n",MAX_LINES); 
       temp += "Do you wish the attachment to be displayed inline?";
	if(KMsgBox::yesNo(0,"?",temp) == 1)
	  {text += "<br><hr><br>";  // Return text.
	  return text;}
	else // We want the icon to be displayed 
	  {QFile *file = new QFile(KApplication::kdedir()+"/share/mimelnk/" 
				   + type + subType); // Search for mimetype.
	  if(!file->open(IO_ReadOnly)) // if does not exist 
	    {file = new QFile(KApplication::kdedir()+"/share/mimelnk" 
			      + "/text/plain.kdelnk" ); // use text/plain  
	    assert(file->open(IO_ReadOnly));
	    printf("Not a  registered mimetype\n");
	    }
	  
	  QTextStream pstream(file);
	  KConfig config(&pstream);
	  config.setGroup("KDE Desktop Entry");
	  QString icon = config.readEntry("Icon");
	  if(icon.isEmpty())
	  icon = KApplication::kdedir()+ "/share/icons/unknown.xpm";
	  else
	    icon.prepend(KApplication::kdedir()+ "/share/icons/");
	  QString comment = config.readEntry("Comment");
	  file->close();
	  text = "<A HREF=\"" + pnumstring + "\"><IMG SRC=" + icon 
	    + ">"+ comment +"</A>";
	  text += "<br><hr><br>";
	  return text;
	  }
      }
    else
      {text += "<br><hr><br>";
      return text;}
    
    }
  else if(type == "message")// If content type is message just display the text.
    {text += "<br><hr><br>";
    return text;
    }

  else
    {QString icon;
     QFile *file = new QFile(KApplication::kdedir()+"/share/mimelnk/" 
			     + type + subType); // Search for mimetype.
     if(file->open(IO_ReadOnly)) // if mimetype exists                        
        {QTextStream pstream(file);         
	KConfig config(&pstream);
	config.setGroup("KDE Desktop Entry");
	icon = config.readEntry("Icon");
	if(icon.isEmpty()) // If no icon specified.
	  icon = KApplication::kdedir()+ "/share/icons/unknown.xpm";
	else
	  icon.prepend(KApplication::kdedir()+ "/share/icons/"); // take it
	QString comment = config.readEntry("Comment");
	file->close();
	text = "<A HREF=\"" + pnumstring + "\"><IMG SRC=" + icon 
	  + ">" + comment + "</A>";
	text += "<br><hr><br>";
	return text;
	}
     else
       {icon = KApplication::kdedir()+ "/share/icons/unknown.xpm";
	printf("Not a registered mimetype\n");
	text = "<A HREF=\"" + pnumstring + "\"><IMG SRC=" + icon + ">Unknown</A>";
	text += "<br><hr><br>";
	return text;
       }
    }

  return "";
    
}

void KMReaderView::initKMimeMagic()
{
  // Magic file detection init
  QString mimefile = kapp->kdedir();
  mimefile += "/share/magic";
  magic = new KMimeMagic( mimefile );
  magic->setFollowLinks( TRUE );
}


QString KMReaderView::decodeString(KMMessagePart *part, QString type)
{
  //    DwString dwstr(data);
  DwString dwDest;
  
  type=type.lower();
  debug("decoding %s",type.data());
  if(type=="base64") 
    {printf("->base64\n");
    DwDecodeBase64(part->body(),dwDest);}
  else if(type=="quoted-printable")
    {printf("->quotedp\n");
    DwDecodeQuotedPrintable(part->body(),dwDest);}
  else if(type=="8bit") 
    {printf("Raw 8 bit data read. Things may look strange");
    dwDest = part->body();
    }
  else if(type == "7bit")
    dwDest = part->body();
  else if(type != "7bit")
    dwDest="Encoding of this attachment is currently not supported!";

  printf("decoded data: %s",dwDest.c_str());
  return dwDest.c_str();
}

//---------------------------------------------------------------------------
// This function is my nightmare!

QString KMReaderView::scanURL(QString text)
{
  // scan for @. Cut out the url than pre-append necessary HREF stuff.
  int pos = 0; // Position where @ is found in the tex. Init to position 0
  int startPos; // Beginnig of url
  int endPos;  // End of url
  int urlLength = 0; // _url Length
  QString deepCopy; // deep copy of text

  while((pos = text.find("@",pos,0)) != -1) // As long as we find @ urls.
    {endPos = text.find(QRegExp("\\s",0,0),pos); // Find end of url;
    startPos = text.findRev(QRegExp("\\s",0,0),pos); // Find beginnig of url
    deepCopy = text.copy(); // make a deep copy of text
    deepCopy.remove(0,startPos+1);
    deepCopy.truncate(endPos);
    cout << "cut deep copy:" << deepCopy << "<---\n";
    text.remove(startPos+1,endPos-1);
    deepCopy = "<A HREF=\"mailto:" + deepCopy + "\">" + deepCopy + "</A>";
    cout << "url:" << deepCopy << "<--\n";
    urlLength = deepCopy.length();
    text.insert(endPos,deepCopy);
    pos = startPos + urlLength;}
  
  return text;
}
    

QString KMReaderView::parseEAddress(QString old)
{
	int pos;
	if((pos = old.find("<",0,0)) == -1)
		{old = "<A HREF=\"mailto:" + old + "\">" + old +"</A>";
		cout << old << "\n";
		return old;}

}


void KMReaderView::replyMessage()
{
	KMComposeWin *c = new KMComposeWin(NULL,NULL,NULL,
					   currentMessage,actReply);
	c->show();
	c->resize(c->size());
}

void KMReaderView::replyAll()
{
	KMComposeWin *c = new KMComposeWin(NULL,NULL,NULL,
					   currentMessage,actReplyAll);
        c->show();
        c->resize(c->size());

}
void KMReaderView::forwardMessage()
{
	KMComposeWin *c = new KMComposeWin(NULL,NULL,NULL,
					   currentMessage,actForward);
	c->show();
	c->resize(c->size());
}


void KMReaderView::nextMessage()
{
	currentIndex++;
	printf("Index (next) : %i\n",currentIndex);
	clearCanvas();
	currentMessage = currentFolder->getMsg(currentIndex);
	parseMessage(currentMessage);
}

void KMReaderView::previousMessage()
{
	if(currentIndex == 1)
		return;
	currentIndex--;
	printf("Index (prev) : %i\n",currentIndex);
	clearCanvas();
	currentMessage = currentFolder->getMsg(currentIndex);
	parseMessage(currentMessage);
}

void KMReaderView::deleteMessage()
{
  /*	printf("Message %i will be deleted\n",currentIndex);
	currentMessage->del();
	currentIndex--;
	currentFolder->expunge();
	nextMessage();*/
}

void KMReaderView::saveMail()
{
	QString mailString;
	QString fileName;
	QFile *file;
	fileName = QFileDialog::getSaveFileName(NULL,"*.kmail",0);
	if(fileName.isEmpty())
		return;
	file = new QFile(fileName);
	if(file->exists())
		{int i = KMsgBox::yesNoCancel(0,"Question?","The file you selected already exists! Overwrite?");
		printf("i = %i\n",i);
		if(i==1);
		else if(i==2)
			{delete file;
			saveMail();}
		else if(i==3)
			{delete file;
			return;	}	
		}
	if(!file->open(IO_ReadWrite))
		{KMsgBox::message(0,"Ouuch","Could not open file!");
		return;}
	mailString = currentMessage->asString();
	file->writeBlock(mailString,mailString.length()-1);
	file->close();

}

void KMReaderView::printMail()
{
  messageCanvas->print();
}

void KMReaderView::slotScrollVert( int _y )
{
        vert->setValue( _y );
}


void KMReaderView::slotScrollHorz( int _x )
{
        horz->setValue( _x );
}

void KMReaderView::slotScrollUpDo()
{
	printf("Entering slotScrollUpDo\n");
	int i = vert->value();
	i--;
	horz->setValue(i);	
}

void KMReaderView::slotScrollLeRi()
{
	printf("Entering slotScrollLeRi\n");
	int i = vert->value();
	i++;
	vert->setValue(i);
}


void KMReaderView::slotDocumentChanged()
{
        if ( messageCanvas->docHeight() > messageCanvas->height() )
                vert->setRange( 0, messageCanvas->docHeight() - messageCanvas->height() );
        else
                vert->setRange( 0, 0 );

        if ( messageCanvas->docWidth() > messageCanvas->width() )
                horz->setRange( 0, messageCanvas->docWidth() - messageCanvas->width() );
        else
                horz->setRange( 0, 0 );
}


void KMReaderView::slotDocumentDone()
{
                vert->setValue( 0 );
}

void KMReaderView::slotOpenAtmnt()
{
  printf("CurrentAtmnt :%i\n",currentAtmnt);
}

void KMReaderView::slotSaveAtmnt()
{
  //((KMReaderWin*)parentWidget())->toDo();
  QString fileName;
  QString text;
  fileName = QFileDialog::getSaveFileName();
  if(fileName.isEmpty())
    return;
  KMMessagePart *p = new KMMessagePart();
  currentMessage->bodyPart(currentAtmnt,p);
  printf("before save decoding\n");
  text = decodeString(p,p->cteStr());
  printf("after save decoding\n");
  QFile *file = new QFile(fileName);
  if(file->exists())
    {KMsgBox::message(0,"Save Body Part","File already exists");
    slotSaveAtmnt();
    }

  if(!file->open(IO_ReadWrite))
    {KMsgBox::message(0,"Save Body Part","Error opening saving File");
    slotSaveAtmnt();
    }

  file->writeBlock(text,text.length());
  file->close();
}

void KMReaderView::slotPrintAtmnt()
{
  ((KMReaderWin*)parentWidget())->toDo();
}

void KMReaderView::slotViewAtmnt()
{
  ((KMReaderWin*)parentWidget())->toDo();
}


void KMReaderView::openURL(const char *url, int)
{
	printf("URL selected\n");
	QString fullURL;
	fullURL = url;
	cout << fullURL << "\n";

        if ( fullURL.find( "http:" ) >= 0 )
                {QString cmd = "kfmclient exec ";
                cmd += fullURL;
                cmd += " Open";
                system( cmd );
		}
        else if ( fullURL.find( "ftp:" ) >= 0 )
        {
                QString cmd = "kfmclient exec ";
                cmd += fullURL;
                cmd += " Open";
                system( cmd );
        }
        else if ( fullURL.find( "mailto:" ) >= 0 )
        {       fullURL.remove(0,7);
                KMComposeWin *w = new KMComposeWin(0,0,fullURL);
	  w->show();
	  w->resize(w->size());
        }

}
void KMReaderView::popupHeaderMenu(const char *_url, const QPoint &cords)
{
	QString url = _url;
        if(!url.isEmpty() && (url.find("@",0,0) != -1) )
                {printf("Mouse was pressed over an url!\n");
                QPopupMenu *p = new QPopupMenu();
                p->insertItem("Add to Addressbook");
                p->insertItem("Properties");
                p->popup(cords,0);}

}

void KMReaderView::popupMenu(const char *_url, const QPoint &cords)
{
        QString temp=_url;
        int number;

        printf("Right mouse button pressed on messageCanvas\n");

        if(temp.isEmpty())
                {printf("Mouse was not over an url!\n"); // Pressed outside url
		QPopupMenu *p = new QPopupMenu();
		p->insertItem("Reply to Sender",this,SLOT(replyMessage()));
		p->insertItem("Reply to All Recipients",this,SLOT(replyAll()));		
		p->insertItem("Forward Message",this,SLOT(forwardMessage()));
		p->insertSeparator();
		QPopupMenu *folderMenu = new QPopupMenu(); // dummy
		folderMenu->insertItem("Inbox");
		folderMenu->insertItem("Sent messages");
		folderMenu->insertItem("Trash");
		p->insertItem("Send Message to Folder",folderMenu);
		p->insertItem("Delete Message");
		p->insertItem("Save Message",this,SLOT(saveMail()));		
		p->insertItem("Print Message",this,SLOT(printMail()));
		p->popup(cords,0);}

        else
		{printf("Mouse was pressed over an url: %s\n",_url); // Pressed over url
		if(temp.find("@",0,0) != -1)
		  {popupHeaderMenu(temp,cords);
		  return;}
	       	temp.replace(QRegExp("file:/"),"");
	       	cout << temp << "\n";
        	number = temp.toUInt();
	       	printf("BodyPart : %i\n",number);
        	currentAtmnt = number;
	       	QPopupMenu *p = new QPopupMenu();
        	p->insertItem("Open...",this,SLOT(slotOpenAtmnt()));
	       	p->insertItem("Print...",this,SLOT(slotPrintAtmnt()));
        	p->insertItem("Save as...",this,SLOT(slotSaveAtmnt()));
	       	p->insertItem("Quick View...",this,SLOT(slotViewAtmnt()));
	       	p->popup(cords,0);}

} 


void KMReaderView::saveURL(int)
{
	printf("Entering saveURL()\n");

}

void KMReaderView::copy()
{
	messageCanvas->getSelectedText(selectedText);
}

void KMReaderView::markAll()
{
}

void KMReaderView::viewSource()
{
	QString text;
	KMProperties *p = new KMProperties(0,0,currentMessage);
	p->show();
	p->resize(p->size());
}


/***************************************************************************/
/***************************************************************************/



KMReaderWin::KMReaderWin(QWidget *, const char *, int msgno = 0,KMFolder *f =0)
	:KTopLevelWidget(NULL)
{

  tempFolder = new KMFolder();
  tempFolder = f;
  setCaption("KMail Reader");

  parseConfiguration();

  newView = new KMReaderView(this,NULL, msgno,f);
  setView(newView);

  setupMenuBar();

  setupToolBar();

  if(!showToolBar)
	enableToolBar(KToolBar::Hide);
  resize(480, 510);
}

void KMReaderWin::show()
{
  KTopLevelWidget::show();
  resize(size());
}


// ******************** Public slots ********************


void KMReaderWin::parseConfiguration()
{
  KConfig *config;
  QString o;
  printf("Entering parseConfig\n");
  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  o = config->readEntry("Reader ShowToolBar");
  if((!o.isEmpty() && o.find("no",0,false)) == 0)
	showToolBar = 0;
  else
	showToolBar = 1;
}

// ***************** Private slots ********************

void KMReaderWin::setupMenuBar()
{
  menuBar = new KMenuBar(this);

  QPopupMenu *menu = new QPopupMenu();
  menu->insertItem("Save...",newView,SLOT(saveMail()),ALT+Key_S);
  menu->insertItem("Address Book...",this,SLOT(toDo()),ALT+Key_B);
  menu->insertItem("Print...",newView,SLOT(printMail()),ALT+Key_P);
  menu->insertItem("Properties",newView,SLOT(viewSource()),ALT+Key_O);
  menu->insertSeparator();
  menu->insertItem("New Composer",this,SLOT(newComposer()),ALT+Key_C);
  menu->insertItem("New Mailreader",this,SLOT(newReader()),ALT+Key_R);
  menu->insertSeparator();
  menu->insertItem("Close",this,SLOT(abort()),CTRL+ALT+Key_C);
  menuBar->insertItem("File",menu);

  menu = new QPopupMenu();
  menu->insertItem("Copy",newView,SLOT(copy()),CTRL+Key_C);
  menu->insertItem("Mark all",newView,SLOT(markAll()));
  menu->insertSeparator();
  menu->insertItem("Find...",this,SLOT(toDo()));
  menuBar->insertItem("Edit",menu);

  menu = new QPopupMenu();
  menu->insertItem("Reply...",newView,SLOT(replyMessage()),ALT+Key_R);
  menu->insertItem("Reply all...", newView,SLOT(replyAll()),ALT+Key_A);
  menu->insertItem("Forward ...",newView,SLOT(forwardMessage()),ALT+Key_F);
  menu->insertSeparator();
  menu->insertItem("Next...",newView,SLOT(nextMessage()),Key_Next);
  menu->insertItem("Previous...",newView,SLOT(previousMessage()),Key_Prior);
  menu->insertSeparator();
  menu->insertItem("Delete...",newView,SLOT(deleteMessage()), Key_Delete);

  menuBar->insertItem("Message",menu);

  menu = new QPopupMenu();
  menu->insertItem("Toggle Toolbar", this, SLOT(toggleToolBar()),ALT+Key_O);

  menuBar->insertItem("Options",menu);

  menuBar->insertSeparator(); 
 
  menu = new QPopupMenu();
  menu->insertItem("Help",this,SLOT(invokeHelp()),ALT+Key_H);
  menu->insertSeparator();
  menu->insertItem("About",this,SLOT(about()));
  menuBar->insertItem("Help",menu);

  setMenu(menuBar);
}

void KMReaderWin::setupToolBar()
{
  KIconLoader *loader = kapp->getIconLoader();
  toolBar = new KToolBar(this);
  
  toolBar->insertButton(loader->loadIcon("kmsave.xpm"),
			0,SIGNAL(clicked()),
			newView,SLOT(saveMail()),TRUE,"Save Mail");
  toolBar->insertButton(loader->loadIcon("kmprint.xpm"),
			      1,SIGNAL(clicked()),
			      newView,SLOT(printMail()),TRUE,"Print");
	toolBar->insertSeparator();
	toolBar->insertButton(loader->loadIcon("kmreply.xpm"),
			      2,SIGNAL(clicked()),
			      newView,SLOT(replyMessage()),TRUE,"Reply");
	toolBar->insertButton(loader->loadIcon("kmreply.xpm"),
			      3,SIGNAL(clicked()),
			      newView,SLOT(replyAll()),TRUE,"Reply all");
	toolBar->insertButton(loader->loadIcon("kmforward.xpm"),
			      4,SIGNAL(clicked()),
			      newView,SLOT(forwardMessage()),TRUE,"Forward");
	toolBar->insertSeparator();
	toolBar->insertButton( loader->loadIcon("kmforward.xpm"),
			      5,SIGNAL(clicked()),
			      newView,SLOT(nextMessage()),TRUE,"Next message");
	toolBar->insertButton(loader->loadIcon("up.xpm"),
			      6,SIGNAL(clicked()),newView,
			      SLOT(previousMessage()),TRUE,"Previous message");
	toolBar->insertSeparator();
	toolBar->insertButton( loader->loadIcon("kmdel.xpm"),
			       7,SIGNAL(clicked()),newView,
			       SLOT(deleteMessage()),TRUE,"Delete Message");
	toolBar->insertSeparator();
	toolBar->insertButton(loader->loadIcon("help.xpm"),
			      8,SIGNAL(clicked()),
			      this,SLOT(invokeHelp()),TRUE,"Help");

	addToolBar(toolBar);
}

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

void KMReaderWin::closeEvent(QCloseEvent *e)
{
	KTopLevelWidget::closeEvent(e);
	delete this;
	KConfig *config = new KConfig();
	config = KApplication::getKApplication()->getConfig();
	config->setGroup("Settings");
	if(showToolBar)
		config->writeEntry("Reader ShowToolBar","yes");
	else
		config->writeEntry("Reader ShowToolBar","no");
	config->sync();
	
}


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

  connect(tabDialog,SIGNAL(applyButtonPressed()),qApp,SLOT(quit()) );
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
	temp.sprintf("Subject: %s", tempMes->subject());
	p.drawText(60, 30, temp);
	temp.sprintf("From: %s", tempMes->from());
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
	temp.sprintf("Sent: %s", tempMes->dateStr());
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



















