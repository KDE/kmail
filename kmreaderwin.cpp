#include "kmreaderwin.moc"
#define FORWARD 0
#define REPLY 1
#define REPLYALL 2

KMReaderView::KMReaderView(QWidget *parent =0, const char *name = 0, int msgno = 0,Folder *f = 0)
	:QWidget(parent,name)
{
	printf("Entering view:  msgno = %i \n",msgno);

	currentFolder = new Folder();
	currentFolder = f;

	currentMessage = new Message();
        currentIndex = msgno;

	if(f !=0)
		currentMessage = f->getMsg(msgno);
	else 
		currentMessage= NULL;

	printf("After getmsg\n");	

	// Let's initialize the HTMLWidget

	theCanvas = new KHTMLWidget(this,0,"/kde/lib/pics/");
	theCanvas->setURLCursor(upArrowCursor);
	theCanvas->resize(parent->width()-16,parent->height()-16);
	connect(theCanvas,SIGNAL(URLSelected(const char *,int)),this,SLOT(openURL(const char *,int)));
	vert = new QScrollBar( 0, 0, 12, theCanvas->height()-16, 0,
                        QScrollBar::Vertical, this, "vert" );
        horz = new QScrollBar( 0, 0, 24, theCanvas->width()-16, 0,
                        QScrollBar::Horizontal, this, "horz" );	
	connect( theCanvas, SIGNAL( scrollVert( int ) ), SLOT( slotScrollVert(int)));
        connect( theCanvas, SIGNAL( scrollHorz( int ) ), SLOT( slotScrollHorz(int)));
        connect( vert, SIGNAL(valueChanged(int)), theCanvas, SLOT(slotScrollVert(int)));
        connect( horz, SIGNAL(valueChanged(int)), theCanvas, SLOT(slotScrollHorz(int)));
	connect( theCanvas, SIGNAL( documentChanged() ), SLOT( slotDocumentChanged() ) );
        connect( theCanvas, SIGNAL( documentDone() ), SLOT( slotDocumentDone() ) );	

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

	theCanvas->begin();
	theCanvas->write("<HTML><BODY BGCOLOR=WHITE></BODY></HTML>");
	theCanvas->end();
	theCanvas->parse();
}

void KMReaderView::updateDisplay()
{
}


// ********************** Protected **********************

void KMReaderView::resizeEvent(QResizeEvent *)
{
  theCanvas->setGeometry(0,0,this->width()-16,this->height()-16);
  horz->setGeometry(0,height()-16,width()-16,16);
  vert->setGeometry(width()-16,0,16,height());
}


// ******************* Private slots ********************

void KMReaderView::parseMessage(Message *message)
{
	QString fromStr;
	QString subjStr;
	QString text;
	QString header;
	char sfrom[256];
	char ssubj[512];
	ulong length;
	const char *B[] = BODYTYPE;
	const char *E[] = ENCODING;

	currentMessage = message; // To make sure currentMessage is set.

	KConfig *config = new KConfig();
	config = KApplication::getKApplication()->getConfig();
	text = config->readEntry("Header");
	if( !text.isEmpty() && text.find("full",0,false) == 0 )
		displayFull = true;
	else
		displayFull = false;

	int noAttach;

	if(!(noAttach =message->numAttch()))
		text = message->getText(&length);
	else
		{text = message->getText(&length);
		text.truncate(length);	
		}          	

	header = message->getHeader();

	// Ok. Convert the text to html
	
	header.replace(QRegExp("\n"),"<BR>");
	text.replace(QRegExp("\n"),"<BR>");

	theCanvas->begin();
	theCanvas->write("<HTML><HEAD><TITLE></TITLE></HEAD>");
	theCanvas->write("<BODY BGCOLOR=WHITE>");
	if(displayFull)
		{theCanvas->write(header);
		theCanvas->write(text);
		}
	else
		{message->getFrom(sfrom);
		message->getSubject(ssubj);
		char sdate[256];
		char scc[256];
		QString dateStr;
		QString ccStr;
		message->getCc(scc);
		message->getLongDate(sdate);
		dateStr.sprintf("Date: %s<br>",sdate);
		fromStr.sprintf("From: %s<br>",sfrom);
		ccStr.sprintf("Cc: %s<br>",scc);
	        subjStr.sprintf("Subject: %s<br><P>",ssubj);
		theCanvas->write(dateStr);
		theCanvas->write(fromStr);
		theCanvas->write(ccStr);
		theCanvas->write(subjStr);
		theCanvas->write(text);
		}

	if(noAttach != 0)
		{while(noAttach > 0)
			{printf("Attach : %i\n",noAttach);
			printf("OK: Writing image for attachment\n");
			Attachment *atmnt = new Attachment();
			theCanvas->write("<hr><center><table><td><tr><IMG SRC=\"");
			QString t;
			t="/cs/kmail-970606/edit.jpg";
			theCanvas->write(t);
			theCanvas->write("\">");
			theCanvas->write("<br><A HREF=\"");
			t.sprintf("%i",noAttach);
			theCanvas->write(t);
			theCanvas->write("\">Part 1.");
			t.sprintf("%i</A>",noAttach);
			theCanvas->write(t);
			theCanvas->write("</td>");
			theCanvas->write("<td ALIGN=LEFT>Name:");
			atmnt = message->getAttch(noAttach);
			t = atmnt->getFilename();	
			theCanvas->write(t);
			theCanvas->write("<br>Type:");
			t.sprintf("%s", B[atmnt->getType()]);
			theCanvas->write(t);
			theCanvas->write("<br>Encoding:");
			t.sprintf("%s",E[atmnt->getEncoding()]);
			theCanvas->write(t);
			theCanvas->write("<br></td></tr></table></center>");
			noAttach--;
			delete atmnt;
			}
		}		
	theCanvas->write("</BODY></HTML>");
	theCanvas->end();
	theCanvas->parse();
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
  QString pwd;
  QString file;
  char buf[511];

  pwd.sprintf("%s",getcwd(buf,sizeof(buf)));
  file = QFileDialog::getSaveFileName(pwd,0,this);

  if(!file.isEmpty())
    {printf("EMail will be saved\n");
    QFile *f = new QFile(file);
    if(!f->open(IO_ReadWrite))
	return;
    else
	   f->close();

     Folder *saveFolder = new Folder();
     if(!saveFolder->open(file))
		{KMsgBox::message(0,"Ouch","SaveFile could not be opened\n");
		return;
		}
    if(!saveFolder->putMsg(currentMessage))
		{KMsgBox::message(0,"Ouch","Could not save message\n");
		saveFolder->close();
		return;
		}
    saveFolder->close();	
    }

  else
    {}

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
        if ( theCanvas->docHeight() > theCanvas->height() )
                vert->setRange( 0, theCanvas->docHeight() - theCanvas->height() );
        else
                vert->setRange( 0, 0 );

        if ( theCanvas->docWidth() > theCanvas->width() )
                horz->setRange( 0, theCanvas->docWidth() - theCanvas->width() );
        else
                horz->setRange( 0, 0 );
}


void KMReaderView::slotDocumentDone()
{
                vert->setValue( 0 );
}


void KMReaderView::openURL(const char *url, int)
{
	QString temp;
	int number;
	printf("URL selected\n");
	cout << url << "\n";
	temp = url;
	temp.replace(QRegExp(":/"),"");
	cout << temp << "\n";
	number = temp.toUInt();
	printf("Attachment : %i",number);
	saveURL(number);	
}


void KMReaderView::saveURL(int num)
{
	printf("Entering saveURL()\n");
	Attachment *a = new Attachment();
	a = currentMessage->getAttch(num);

	QString pwd;
	QString file;
 	char buf[511];

  	pwd.sprintf("%s",getcwd(buf,sizeof(buf)));
  	file = QFileDialog::getSaveFileName(pwd,0,this);
  	if(!file.isEmpty())
    		{printf("Attachment will be saved\n");
           	a->save(file);	
		}
	else
		{}

	return;

}
/***************************************************************************/
/***************************************************************************/



KMReaderWin::KMReaderWin(QWidget *, const char *, int msgno = 0,Folder *f =0)
	:KTopLevelWidget(NULL)
{

  tempFolder = new Folder();
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


  o = config->readEntry("Header");
  if( !o.isEmpty() && o.find("half",0,false) ==0)
	displayFull = false;
  else
	displayFull = true;
}

// ***************** Private slots ********************

void KMReaderWin::setupMenuBar()
{
  menuBar = new KMenuBar(this);

  QPopupMenu *menu = new QPopupMenu();
  menu->insertItem("Save...",newView,SLOT(saveMail()),ALT+Key_S);
  menu->insertItem("Address Book...",this,SLOT(toDo()),ALT+Key_B);
  menu->insertItem("Print...",this,SLOT(toDo()),ALT+Key_P);
  menu->insertSeparator();
  menu->insertItem("New Composer",this,SLOT(newComposer()),ALT+Key_C);
  menu->insertItem("New Mailreader",this,SLOT(newReader()),ALT+Key_R);
  menu->insertSeparator();
  menu->insertItem("Close",this,SLOT(abort()),CTRL+ALT+Key_C);
  menuBar->insertItem("File",menu);

  menu = new QPopupMenu();
  menu->insertItem("Copy",this,SLOT(toDo()),CTRL+Key_C);
  menu->insertItem("Mark all",this,SLOT(toDo()));
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
	 else pixdir.append("/usr/local/kde");
	pixdir.append("/lib/pics/toolbar/");

	toolBar = new KToolBar(this);

	QPixmap pixmap;

	pixmap.load(pixdir+"mailsave.xpm");
	toolBar->insertItem(pixmap,0,SIGNAL(clicked()),newView,SLOT(saveMail()),TRUE,"Save Mail");
	pixmap.load(pixdir+"fileprint.xpm");
	toolBar->insertItem(pixmap,1,SIGNAL(clicked()),this,SLOT(toDo()),TRUE,"Print");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"reply.xpm");
	toolBar->insertItem(pixmap,2,SIGNAL(clicked()),newView,SLOT(replyMessage()),TRUE,"Reply");
	pixmap.load(pixdir+"reply.xpm");
	toolBar->insertItem(pixmap,3,SIGNAL(clicked()),newView,SLOT(replyAll()),TRUE,"Reply all");
	pixmap.load(pixdir+"kmforward.xpm");
	toolBar->insertItem(pixmap,4,SIGNAL(clicked()),newView,SLOT(forwardMessage()),TRUE,"Forward");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"down.xpm");
	toolBar->insertItem(pixmap,5,SIGNAL(clicked()),newView,SLOT(nextMessage()),TRUE,"Next message");
	pixmap.load(pixdir+"up.xpm");
	toolBar->insertItem(pixmap,6,SIGNAL(clicked()),newView,SLOT(previousMessage()),TRUE,"Previous message");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"maildel.xpm");
	toolBar->insertItem(pixmap,7,SIGNAL(clicked()),newView,SLOT(deleteMessage()),TRUE,"Delete Message");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"help.xpm");
	toolBar->insertItem(pixmap,8,SIGNAL(clicked()),this,SLOT(invokeHelp()),TRUE,"Help");

	addToolBar(toolBar);
}

void KMReaderWin::invokeHelp()
{

  KApplication::getKApplication()->invokeHTMLHelp("","");

}

void KMReaderWin::toDo()
{
	//  KMMainWin::doUnimplemented(); //is private :-(

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

/*void KMReaderWin::setSettings()
{
   setWidget = new QWidget(0,NULL);
   setWidget->setMinimumSize(400,300);
   setWidget->setMaximumSize(400,300);
   setWidget->setCaption("Settings");

   QPushButton *ok_bt = new QPushButton("OK",setWidget,NULL);
   ok_bt->setGeometry(220,240,70,30);
   connect(ok_bt,SIGNAL(clicked()),this,SLOT(applySettings()));

   QPushButton *cancel_bt = new QPushButton("Cancel",setWidget,NULL);
   cancel_bt->setGeometry(310,240,70,30);
   connect(cancel_bt,SIGNAL(clicked()),this,SLOT(cancelSettings()));

   QButtonGroup *btGrp = new QButtonGroup(setWidget,NULL);
   btGrp->setTitle("Options");
   btGrp->setGeometry(20,20,360,200);

   QLabel *headerLabel = new QLabel("The Reader should display",btGrp,NULL);
   headerLabel->setGeometry(20,20,200,30);

   fullHeader = new QRadioButton("'the full header'",btGrp,NULL);
   fullHeader->setGeometry(30,50,150,20);

   halfHeader = new QRadioButton("'the most important'",btGrp,NULL);
   halfHeader->setGeometry(30,80,150,20);

   if(displayFull == false)
        halfHeader->setChecked(true);
   else
        fullHeader->setChecked(true);

   setWidget->show();

}

void KMReaderWin::applySettings()
{
  KConfig *config;
  if(halfHeader->isChecked())
        displayFull=false;
  else
        displayFull=true;

  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");

  if(halfHeader->isChecked())
        {displayFull=false;
        config->writeEntry("Header","half");
        }
  else
        {displayFull=true;
        config->writeEntry("Header","full");
        }

  config->sync();

  newView->updateDisplay();

  delete setWidget;
}

void KMReaderWin::cancelSettings()
{
  delete setWidget;
}
*/

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



