// kmreaderwin.cpp

#include "kmreaderwin.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmreaderwin.moc"

#define FORWARD 0
#define REPLY 1
#define REPLYALL 2

KMReaderView::KMReaderView(QWidget *parent =0, const char *name = 0, int msgno = 0,KMFolder *f = 0)
	:QWidget(parent,name)
{
	printf("Entering view:  msgno = %i \n",msgno);

	kdeDir = getenv("KDEDIR");
	if(!kdeDir)
		{KMsgBox::message(0,"Ouuch","$KDEDIR not set.\nPlease do so");
		qApp->quit();
		}
	picsDir.append(kdeDir);
	picsDir +="/lib/pics";

	currentFolder = new KMFolder();
	currentFolder = f;

	currentMessage = new KMMessage();
        currentIndex = msgno;

	if(f !=0)
		currentMessage = f->getMsg(msgno);
	else 
		currentMessage= NULL;

	printf("After getmsg\n");	

	// Let's initialize the HTMLWidget


	headerCanvas = new KHTMLWidget(this,0,0);
	headerCanvas->resize(parent->width(),parent->height()-100);	

	separator = new QFrame(this);
	separator->setFrameStyle(QFrame::HLine | QFrame::Raised);
	separator->setLineWidth(4);

	messageCanvas = new KHTMLWidget(this,0,picsDir);
	messageCanvas->setURLCursor(upArrowCursor);
	messageCanvas->resize(parent->width()-16,parent->height()-110); //16
	connect(messageCanvas,SIGNAL(URLSelected(const char *,int)),this,SLOT(openURL(const char *,int)));
	connect(messageCanvas,SIGNAL(popupMenu(const char *, const QPoint &)), SLOT(popupMenu(const char *, const QPoint &)));
	vert = new QScrollBar( 0, 110, 12, messageCanvas->height()-110, 0,
                        QScrollBar::Vertical, this, "vert" );
        horz = new QScrollBar( 0, 0, 24, messageCanvas->width()-16, 0,
                        QScrollBar::Horizontal, this, "horz" );	
	connect( messageCanvas, SIGNAL( scrollVert( int ) ), SLOT( slotScrollVert(int)));
        connect( messageCanvas, SIGNAL( scrollHorz( int ) ), SLOT( slotScrollHorz(int)));
        connect( vert, SIGNAL(valueChanged(int)), messageCanvas, SLOT(slotScrollVert(int)));
        connect( horz, SIGNAL(valueChanged(int)), messageCanvas, SLOT(slotScrollHorz(int)));
	connect( messageCanvas, SIGNAL( documentChanged() ), SLOT( slotDocumentChanged() ) );
        connect( messageCanvas, SIGNAL( documentDone() ), SLOT( slotDocumentDone() ) );	



	// Puh, okay this is done

	QAccel *accel = new QAccel( this );       
        accel->connectItem( accel->insertItem(Key_Up),this,SLOT(slotScrollLeRi()) );      
	accel->connectItem( accel->insertItem(Key_Down),this,SLOT(slotScrollUpDo()) );      

	if(currentMessage)
		parseMessage(currentMessage);
	else
		clearCanvas();

	printf("Leaving constructor\n");		
	
}


// *********************** Public slots *******************

void KMReaderView::clearCanvas()
{
	// Produce a white canvas

	headerCanvas->begin(picsDir);
	headerCanvas->write("<HTML><BODY BGCOLOR=WHITE></BODY></HTML>");
	headerCanvas->end();
	headerCanvas->parse();

	messageCanvas->begin(picsDir);
	messageCanvas->write("<HTML><BODY BGCOLOR=WHITE></BODY></HTML>");
	messageCanvas->end();
	messageCanvas->parse();
}

void KMReaderView::updateDisplay()
{
}


// ********************** Protected **********************

void KMReaderView::resizeEvent(QResizeEvent *)
{
  headerCanvas->setGeometry(0,0,this->width(),75);
  separator->setGeometry(0,76,this->width(),4);
  messageCanvas->setGeometry(0,81,this->width()-16,this->height()-87); //16
  horz->setGeometry(0,height()-16,width()-16,16);
  vert->setGeometry(width()-16,81,16,height()-87);
}


// ******************* Private slots ********************

void KMReaderView::parseMessage(KMMessage *message)
{
	QString fromStr;
	QString subjStr;
	QString text;
	QString header;
	QString dateStr;
	QString ccStr;
	long length;

	currentMessage = message; // To make sure currentMessage is set.

	int noAttach = (message->numBodyParts() <= 1);

	text = message->body(&length);
	if (noAttach) text.truncate(length);	

	headerCanvas->begin(picsDir);
	headerCanvas->write("<HTML><HEAD><TITLE></TITLE></HEAD>");
	headerCanvas->write("<BODY BGCOLOR=WHITE>");

	dateStr.sprintf("Date: %s<br>",message->dateStr());
	fromStr.sprintf("From: %s<br>",message->from());
	ccStr.sprintf("Cc: %s<br>",message->cc());
             subjStr.sprintf("Subject: %s<br><P>",message->subject());

	headerCanvas->write(dateStr);
	headerCanvas->write(fromStr);
	headerCanvas->write(ccStr);
	headerCanvas->write(subjStr);
	headerCanvas->write("</BODY></HTML>");
	headerCanvas->end();
	headerCanvas->parse();

	// Init messageCanvas
        messageCanvas->begin(picsDir);

	// Prepare text

//***** 1. Search for hyperlinks (http, ftp , @) ********//


//  ****** 2. Check if message body is html. Search for <html> tag ********//

	if((text.find(QRegExp("<html>",0,0)) != -1) && (text.find(QRegExp("</html>",0,0)) != -1)) // we found the tags
		printf("Found html tags!\n");

	else
	        {// First of all convert escape sequences etc to html
		text.replace(QRegExp("\n"),"<BR>");
		text.replace(QRegExp("\\x20",FALSE,FALSE),"&nbsp"); // SP
		
		messageCanvas->write("<HTML><HEAD><TITLE></TITLE></HEAD>");
		messageCanvas->write("<BODY BGCOLOR=WHITE>");
		}

	// Okay! Let's write it to the canvas
	messageCanvas->write(text);
		
#ifdef BROKEN
	if(noAttach != 0)
		{while(noAttach > 0)
			{printf("Attach : %i\n",noAttach);
			printf("OK: Writing image for attachment\n");
			Attachment *atmnt = new Attachment();
			messageCanvas->write("<hr><center><table><td><tr><IMG SRC=\"");
			QString t.append(kdeDir);
			t += "/lib/pics/kmattach.jpg";
			messageCanvas->write(t);
			messageCanvas->write("\">");
			messageCanvas->write("<br><A HREF=\"");
			t.sprintf("%i",noAttach);
			messageCanvas->write(t);
			messageCanvas->write("\">Part 1.");
			t.sprintf("%i</A>",noAttach);
			messageCanvas->write(t);
			messageCanvas->write("</td>");
			messageCanvas->write("<td ALIGN=LEFT>Name:");
			atmnt = message->getAttch(noAttach);
			t = atmnt->getFilename();	
			messageCanvas->write(t);
			messageCanvas->write("<br>Type:");
			t.sprintf("%s", B[atmnt->getType()]);
			messageCanvas->write(t);
			messageCanvas->write("<br>Encoding:");
			t.sprintf("%s",E[atmnt->getEncoding()]);
			messageCanvas->write(t);
			messageCanvas->write("<br></td></tr></table></center>");
			noAttach--;
			delete atmnt;
			}
		}		
#endif

	messageCanvas->write("</BODY></HTML>");
	messageCanvas->end();
	messageCanvas->parse();
	printf("Leaving parsing\n");
}

void KMReaderView::replyMessage()
{
	printf("Entering reply\n");
	KMComposeWin *c = new KMComposeWin(NULL,NULL,NULL,currentMessage,REPLY);
	c->show();
	c->resize(c->size());
	printf("Leaving reply()\n");
}

void KMReaderView::replyAll()
{
	KMComposeWin *c = new KMComposeWin(NULL,NULL,NULL,currentMessage,REPLYALL);
        c->show();
        c->resize(c->size());
        printf("Leaving replyAll()\n");

}
void KMReaderView::forwardMessage()
{
	KMComposeWin *c = new KMComposeWin(NULL,NULL,NULL,currentMessage,FORWARD);
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
	printf("Message %i will be deleted\n",currentIndex);
	currentMessage->del();
	currentIndex--;
	currentFolder->expunge();
	nextMessage();
}

void KMReaderView::saveMail()
{

}

void KMReaderView::printMail()
{
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
        {
                KMsgBox::message(0,"Ouuch","Sorry not yet implemented!");
        }

}

void KMReaderView::popupMenu(const char *_url, const QPoint &cords)
{
        char buf[30];
        QString temp;
        int number;

        printf("Right mouse button pressed\n");

        strcpy(buf,"");
        if(!strcpy(buf,_url))
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
		p->popup(cords,0);
                }
        else
		{printf("Mouse was pressed over an url: %s\n",_url); // Pressed over url
	        temp = _url;
        	temp.replace(QRegExp("file:/"),"");
	        cout << temp << "\n";
        	number = temp.toUInt();
	        printf("Attachment : %i",number);
        	currentAtmnt = number;
	        QPopupMenu *p = new QPopupMenu();
        	p->insertItem("Open...",this,SLOT(openAtmnt()));
	        p->insertItem("Print...",this,SLOT(printAtmnt()));
        	p->insertItem("Save as...",this,SLOT(saveAtmnt()));
	        p->insertItem("Quick View...",this,SLOT(viewAtmnt()));
	        p->popup(cords,0);
		}
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
	QString pixdir = "";   // pics dir code "inspired" by kghostview (thanks)
	char *kdedir = getenv("KDEDIR");
	if (kdedir) pixdir.append(kdedir);
	 else 
		{KMsgBox::message(0,"Oucch","$KDEDIR not set. Please do so");
		qApp->quit();
		}
	pixdir += "/lib/pics/toolbar/";



	toolBar = new KToolBar(this);

	QPixmap pixmap;

	pixmap.load(pixdir+"kmsave.xpm");
	toolBar->insertButton(pixmap,0,SIGNAL(clicked()),newView,SLOT(saveMail()),TRUE,"Save Mail");
	pixmap.load(pixdir+"kmprint.xpm");
	toolBar->insertButton(pixmap,1,SIGNAL(clicked()),newView,SLOT(printMail()),TRUE,"Print");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"kmreply.xpm");
	toolBar->insertButton(pixmap,2,SIGNAL(clicked()),newView,SLOT(replyMessage()),TRUE,"Reply");
	pixmap.load(pixdir+"kmreply.xpm");
	toolBar->insertButton(pixmap,3,SIGNAL(clicked()),newView,SLOT(replyAll()),TRUE,"Reply all");
	pixmap.load(pixdir+"kmforward.xpm");
	toolBar->insertButton(pixmap,4,SIGNAL(clicked()),newView,SLOT(forwardMessage()),TRUE,"Forward");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"down.xpm");
	toolBar->insertButton(pixmap,5,SIGNAL(clicked()),newView,SLOT(nextMessage()),TRUE,"Next message");
	pixmap.load(pixdir+"up.xpm");
	toolBar->insertButton(pixmap,6,SIGNAL(clicked()),newView,SLOT(previousMessage()),TRUE,"Previous message");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"kmdel.xpm");
	toolBar->insertButton(pixmap,7,SIGNAL(clicked()),newView,SLOT(deleteMessage()),TRUE,"Delete Message");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"help.xpm");
	toolBar->insertButton(pixmap,8,SIGNAL(clicked()),this,SLOT(invokeHelp()),TRUE,"Help");

	addToolBar(toolBar);
}

void KMReaderWin::invokeHelp()
{

  KApplication::getKApplication()->invokeHTMLHelp("","");

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



