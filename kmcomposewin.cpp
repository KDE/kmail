#include <unistd.h>
#include <qfiledlg.h>
#include <qaccel.h>
#include <qlabel.h>
#include "kmcomposewin.moc"
#include "kmmainwin.h"
#include "kmmessage.h"
#include <iostream.h>
#include <qwidget.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mimelib/string.h>

KMComposeView::KMComposeView(QWidget *parent, const char *name, QString emailAddress, KMMessage *message, int action) : QWidget(parent,name)
{
	printf("Entering composeView\n");
	
	attWidget = NULL;
	if (message) currentMessage = message;
	else currentMessage = new KMMessage(); 
	indexAttachment =0;

	toLEdit = new QLineEdit(this);
	if (emailAddress) 
		toLEdit->setText(emailAddress);
	ccLEdit = new QLineEdit(this);
	subjLEdit = new QLineEdit(this);

	QLabel *label = new QLabel(toLEdit, "&To:", this);
	label->setGeometry(14,10,50,15);

	label = new QLabel(subjLEdit, "&Cc:", this);
	label->setGeometry(14,45,50,20);

	label = new QLabel(ccLEdit, "&Subject:", this);
	label->setGeometry(14,80,50,20);

	editor = new KEdit(0,this);

	if(message && action==FORWARD)
	  {printf("Message will be forwarded\n");
	  forwardMessage();	
	  }
	else if(message && action==REPLY)
		{ printf("Message will be a reply message\n");
		  replyMessage();
		}
	else if(message && action ==REPLYALL)
		{printf("Reply all (compo)\n");
		replyAll();
		}
	else
		printf("Normal message\n");

	parseConfiguration();	

	printf("Leaving constructor\n");		

}

KMComposeView::~KMComposeView()
{}
// ******************  Public slots *************

void KMComposeView::printIt()
{
	// QPrinter is extremly broken. Even the Trolls admitted
	// that. They said they would fix it in version 1.3.
	// For now printing is crap.

    QPrinter *printer = new QPrinter();
    if ( printer->setup(this) ) {
        QPainter paint;
        paint.begin( printer );
	QString text;
	//text.sprintf("From: %s \n",EMailAddress); 
	text += "To: \n";
	text += toLEdit->text();
	text += "\nSubject: ";
	text += subjLEdit->text();
	text += "Date: \n\n";
	text += editor->text();
	text.replace(QRegExp("\n"),"\n");
        paint.drawText(30,30,text);
        paint.end();
    }
   delete printer;
}


void KMComposeView::attachFile()
{
#ifdef BROKEN
	QString atmntFile;
        const char *B[] = BODYTYPE;
	struct stat atmntStat;
	QString fileName;


        QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
        d->setCaption("Attach File");
        if (d->exec()) 
		{atmntFile = d->selectedFile();
		Attachment *a = new Attachment();
		if(!a->guess(atmntFile))
			KMsgBox::message(0,"Ouch","Trouble guessing attachment!\n");			
		else
			{printf("Guessing successfull...\n");
			if(!attWidget)
				{printf("Creating Attachment Widget\n");
				attWidget = new KTabListBox(this);
				attWidget->setNumCols(3);
				attWidget->setColumn(0,"Filename",width()/2-20);
				attWidget->setColumn(1,"File Type",width()/4); 
				attWidget->setColumn(2,"File Size",width()/4);
				cout << "Attachment File: " << atmntFile << "\n";
				fileName =atmntFile.copy();
				attachmentList.append( new KMAttachmentItem(fileName,indexAttachment));
				KMAttachmentItem *itm;
 				printf("About to display list:\n\n");
 				for ( itm=attachmentList.first(); itm != 0; itm = attachmentList.next() )
 					cout <<  "FileName: " << itm->fileName << "\tIndex: " <<  itm->index << "\n";
				printf("\nDone displaying list\n");
				::stat(atmntFile,&atmntStat);
				QString temp;
				temp.sprintf("%s",B[a->getType()]);
				atmntFile += "\t";
				atmntFile += temp;
				atmntFile += "\t";
				temp.sprintf("%ld bytes",atmntStat.st_size);
				atmntFile +=temp;
				attWidget->insertItem(atmntFile);
				indexAttachment++;
				attWidget->setAutoUpdate(TRUE);
				attWidget->show();
				connect(attWidget,SIGNAL(highlighted(int,int)),SLOT(detachFile(int,int)));
				resizeEvent(NULL);
				delete itm;
				atmntFile="";
				}
			else
				{printf("We already have a widget\n");
				cout << "Attachment File: " << atmntFile << "\n";
				fileName = atmntFile.copy();
				attachmentList.append( new KMAttachmentItem(fileName,indexAttachment));
				KMAttachmentItem *itm;
				printf("About to display list:\n\n");
	 			for ( itm=attachmentList.first(); itm != 0; itm = attachmentList.next())
					cout <<  "FileName: " << itm->fileName << "\tIndex: " <<  itm->index << "\n";
				printf("\nDone displaying list\n");
				::stat(atmntFile,&atmntStat);
     				QString temp;
     				temp.sprintf("%s",B[a->getType()]);
				atmntFile += "\t";
     				atmntFile += temp;
     				atmntFile += "\t";
     				temp.sprintf("%ld bytes",atmntStat.st_size);
     				atmntFile +=temp;
     				attWidget->insertItem(atmntFile);
     				indexAttachment++;
    				attWidget->setAutoUpdate(TRUE);
     				attWidget->show();
     				connect(attWidget,SIGNAL(selected(int,int)),SLOT(detachFile(int,int)));
				attWidget->repaint();
				delete itm;
				atmntFile="";
				
				}
			fileName ="";	
			delete a;
			}
		}
        delete d;
#endif

}

void KMComposeView::sendIt()
{
	KConfig *config;
	QString option;

	// Now, what are we going to do: sendNow() or sendLater()?

	config = KApplication::getKApplication()->getConfig();
	config->setGroup("Settings");
	option = config->readEntry("Send Button");
	if(!strcmp(option,"now"))
		sendNow();
	else
		toDo();


}

void KMComposeView::sendNow()
{
	KMMessage *msg = new KMMessage();
	msg = currentMessage;

	// Now all items in the attachment queue are being displayed.

	KMAttachmentItem *itm;
 	printf("About to display list (sendNow()):\n\n");
 	for ( itm=attachmentList.first(); itm != 0; itm = attachmentList.next())
	 	cout <<  "FileName: " << itm->fileName << "\tIndex: " <<  itm->index << "\n";
	printf("\nDone displaying list\n");

	// All attachments in the queue are being attached here.
#if 0
	if(indexAttachment !=0)
	        {int x;
		QList<KMAttachmentItem> tempList;
		tempList = attachmentList;
		tempList.first();
        	for(x=1; x <= indexAttachment; x++)
                	{printf("Attaching No.%i\n",x);
			Attachment *a = new Attachment();
			if(!a->guess(tempList.current()->fileName))
				printf("Error\n");
			msg->attach(a);
			tempList.next();
			delete a;
			}
		printf("Attached files\n");
		}
#endif
	QString temp=toLEdit->text();
	if (temp.isEmpty()) {
		KMsgBox::message(0,"Ouch","No recipients defined. aborting ....");
		return;
		}

	// Now, I have a problems with the CRLF. Everything works fine under 
	// Unix (of course ;-) ) but under MS-Windowz the CRLF is not inter-
	// preted. Why??

	temp = editor->text();
	temp.replace(QRegExp("\r"),"\r\n");
	
	// The the necessary Message() stuff

	msg->setFrom(EMailAddress);
	msg->setTo(toLEdit->text());
	msg->setCc(ccLEdit->text());
	msg->setSubject(subjLEdit->text());
	msg->setBody(temp);
	//msg->sentSMTP();
	delete msg;
	((KMComposeWin *)parentWidget())->close();
}

// ********************* Private slots ****************

void KMComposeView::parseConfiguration()
{
	printf("View : parseConfiguration\n");
        KConfig *config = new KConfig();
        config = KApplication::getKApplication()->getConfig();
	config->setGroup("Settings");
        QString o = config->readEntry("Signature");
        if( !o.isEmpty() && o.find("auto",0,false) ==0)
                appendSignature();

	config->setGroup("Network");
	SMTPServer = config->readEntry("SMTP Server");
	cout << SMTPServer << "\n";

	config->setGroup("Identity");
	EMailAddress = config->readEntry("Email Address");
	cout << EMailAddress << "\n";

        printf("Leaving View: parseConfiguration\n");
 
}

void KMComposeView::forwardMessage()
{
	  printf("Entering forwarding message\n");
          QString temp;
          long length;
          temp.sprintf("Fwd: %s",currentMessage->subject());
          subjLEdit->setText(temp);
          temp ="\n\n---------Forwarded message-----------";
          editor->append(temp);
          temp.sprintf("Date: %s",currentMessage->dateStr());
          editor->append(temp);
          temp.sprintf("From: %s",currentMessage->from());
          editor->append(temp);
          temp.sprintf("To: %s",currentMessage->to());
          editor->append(temp);
          temp.sprintf("Cc: %s",currentMessage->cc());
          editor->append(temp);
          temp.sprintf("Subject: %s\n", currentMessage->subject());
          editor->append(temp);

	  temp = currentMessage->body(&length);
          if (currentMessage->numBodyParts() > 1) temp.truncate(length);
	  editor->append(temp);

          editor->update();
          editor->repaint();
	
}

void KMComposeView::replyAll()
{
          KMMessage *msg = new KMMessage();
          QString temp;
          long length;
          int lines;
          temp.sprintf("Re: %s",currentMessage->subject());
          toLEdit->setText(currentMessage->from());
	  ccLEdit->setText(currentMessage->cc());
          subjLEdit->setText(temp);

	  temp.sprintf("\nOn %s %s wrote:\n", currentMessage->dateStr(), 
		       currentMessage->from());
	  editor->append(temp);

	  temp = currentMessage->body(&length);
          if (currentMessage->numBodyParts() > 1) temp.truncate(length);
	  editor->append(temp);

          lines = editor->numLines();
          printf("We are here\n");
          for(int x=2;x < lines;x++)
                {editor->insertAt("> ",x,0);
                }
          editor->update();
	  
          currentMessage = currentMessage->reply();
          delete msg;


}

void KMComposeView::replyMessage()
{
	  KMMessage *msg = new KMMessage();
          QString temp;
          long length;
          int lines;

          temp.sprintf("Re: %s",currentMessage->subject());
          toLEdit->setText(currentMessage->from());
          subjLEdit->setText(temp);

	  temp.sprintf("\nOn %s %s wrote:\n", currentMessage->dateStr(), 
		       currentMessage->from());
	  editor->append(temp);

	  temp = currentMessage->body(&length);
          if (currentMessage->numBodyParts() > 1) temp.truncate(length);
	  editor->append(temp);

          lines = editor->numLines();
          printf("We are here\n");
          for(int x=2;x < lines;x++)
                {editor->insertAt("> ",x,0);
                }
          editor->update();
	  currentMessage = currentMessage->reply();
  	  delete msg;

}


void KMComposeView::undoEvent()
{
}

void KMComposeView::copyText()
{
  editor->copyText();
}

void KMComposeView::cutText()
{
  editor->cut();
}

void KMComposeView::pasteText()
{
  editor->paste();
}

void KMComposeView::markAll()
{
  editor->selectAll();
}

void KMComposeView::find()
{
  editor->Search();
}



void KMComposeView::detachFile(int index, int col)
{
	col = col;
	printf("Detaching file at index : %i\n",index);
	attachmentList.remove(index);
	attWidget->removeItem(index);
	indexAttachment--;
	if(!indexAttachment)
		{printf("Removing attachWidget\n");
		delete attWidget;
		attWidget=NULL;
		resize(size());
		}
	
}

void KMComposeView::insertFile()
{
      	QString iFileString;
	QString textString;
	QFile *iFileName;
	char buf[255];
	int col, line;
       	QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
        d->setCaption("Insert File");
        if (d->exec()) 
		{iFileString = d->selectedFile();
		 iFileName = new QFile(iFileString);
		 iFileName->open(IO_ReadOnly);
		 iFileName->at(0);
  		 while(iFileName->readLine(buf,254) != 0)
 			textString.append(buf);			
		iFileName->close();
	        editor->getCursorPosition(&line,&col);
		editor->insertAt(textString,line,col);
		editor->update();
		editor->repaint(); 
		}
}	

void KMComposeView::toDo()
{
   KMsgBox::message(this,"Ouch",
			 "Not yet implemented!\n"
			 "We are sorry for the inconvenience :-)",1);
}

void KMComposeView::resizeEvent(QResizeEvent *)
{
  toLEdit->setGeometry(70,10,width()-80,25);
  ccLEdit->setGeometry(70,45,width()-80,25);
  subjLEdit->setGeometry(70,80,width()-80,25);
  if(!attWidget)
  	editor->setGeometry(10,115,width()-20,height()-125); //115
  else
	{editor->setGeometry(10,115,width()-20,height()-200); //115
	attWidget->setGeometry(10,height()-77,width()-20,67);
	attWidget->setColumn(0,"Filename",width()/2-20);
        attWidget->setColumn(1,"File Type",width()/4);
        attWidget->setColumn(2,"File Size",width()/4);	
	attWidget->repaint();
	}
}

void KMComposeView::newComposer()
{
  KMComposeWin *newComposer = new KMComposeWin(NULL,NULL,NULL,NULL);
  newComposer->show();
  newComposer->resize(newComposer->size());

}




/*void KMComposeWin::setSettings()
{
   setWidget = new QWidget(0,NULL);
   setWidget->setMinimumSize(400,320);
   setWidget->setMaximumSize(400,320);
   setWidget->setCaption("Settings");
 
   QPushButton *ok_bt = new QPushButton("OK",setWidget,NULL);
   ok_bt->setGeometry(220,260,70,30);
   connect(ok_bt,SIGNAL(clicked()),this,SLOT(applySettings()));

   QPushButton *cancel_bt = new QPushButton("Cancel",setWidget,NULL);
   cancel_bt->setGeometry(310,260,70,30);
   connect(cancel_bt,SIGNAL(clicked()),this,SLOT(cancelSettings()));			

   QButtonGroup *btGrp = new QButtonGroup(setWidget,NULL);
   btGrp->setGeometry(20,20,360,110);   
  
   QButtonGroup *btGrpII = new QButtonGroup(setWidget,NULL);	
   btGrpII->setGeometry(20,140,360,110); 
 
   QLabel *sendLabel = new QLabel("Send button in the toolbar is a",btGrp,NULL);
   sendLabel->setGeometry(20,10,200,30);

   QRadioButton *isNow = new QRadioButton("'send now' Button",btGrp,NULL);
   isNow->setGeometry(30,40,150,20);

   isLater = new QRadioButton("'send later' Button",btGrp,NULL); 
   isLater->setGeometry(30,70,150,20); 

   QLabel *sigLabel = new QLabel("Signature is appended",btGrpII,NULL);
   sigLabel->setGeometry(20,10,150,20);

   QRadioButton *autoSig = new QRadioButton("'automatically'",btGrpII,NULL);
   autoSig->setGeometry(30,40,150,20);

   manualSig = new QRadioButton("'manually'",btGrpII,NULL);
   manualSig->setGeometry(30,70,150,20);

   if(sendButton == false)
	isLater->setChecked(true);
   else
	isNow->setChecked(true);

   if(sigStatus == true)
	manualSig->setChecked(true);
   else
	autoSig->setChecked(true);

   setWidget->show();

}

void KMComposeWin::applySettings()
{
  KConfig *config;

  if(isLater->isChecked())
	sendButton=false;
  else
	sendButton=true;

  if(manualSig->isChecked())
	sigStatus=true;
  else
	sigStatus=false;


  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  
  if(isLater->isChecked())
	{sendButton=false;
	config->writeEntry("Send Button","later");
	}
  else
	{sendButton=true;
	config->writeEntry("Send Button","now");
	}

  if(manualSig->isChecked())
	config->writeEntry("Signature","manual");
  else
	config->writeEntry("Signature","auto");

  config->sync();

  delete setWidget;
}

void KMComposeWin::cancelSettings()
{
  delete setWidget;
}

*/

KMComposeWin::KMComposeWin(QWidget *, const char *name, QString emailAddress, KMMessage
*message, int action) : KTopLevelWidget(name)
{
  setCaption("KMail Composer");

  parseConfiguration();

  composeView = new KMComposeView(this,NULL,emailAddress,message,action);
  setView(composeView);

  setupMenuBar();

  setupToolBar();

  printf("toolBarStatus in Constr. = %i\n",toolBarStatus);
  if(toolBarStatus==false)
	enableToolBar(KToolBar::Hide);	
	
  resize(480, 510);
}

void KMComposeWin::show()
{
  KTopLevelWidget::show();
  resize(size());
}


void KMComposeWin::parseConfiguration()
{
  KConfig *config;
  QString o;

  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  o = config->readEntry("ShowToolBar");
  if((!o.isEmpty() && o.find("no",0,false)) == 0)
	{toolBarStatus = false;
	printf("tStat = %i\n",toolBarStatus);
	}
  else
	{toolBarStatus = true;
	printf("tStat = %i\n",toolBarStatus);
	}

  o = config->readEntry("Signature");
  if((!o.isEmpty() && o.find("auto",0,false)) == 0)
	{sigStatus = false;
         printf("sigStatus = %i\n",sigStatus);
	}
  else
	{sigStatus = true;
	printf("sigStatus = %i\n",sigStatus);
	}

  o = config->readEntry("Send button");
  if((!o.isEmpty() && o.find("later",0,false)) == 0)
	{sendButton = false;
	printf("sendButton = %i\n",sendButton);
	}
  else
	sendButton = true;
}	

/**********************************************************************************************/
void KMComposeWin::setupMenuBar()
{
  menuBar = new KMenuBar(this);

  QPopupMenu *fmenu = new QPopupMenu();
  fmenu->insertItem("Send",composeView,SLOT(sendNow()), ALT+Key_X);
  fmenu->insertItem("Send &later",composeView,SLOT(toDo()),ALT+Key_L);
  fmenu->insertSeparator();
  fmenu->insertItem("Address &Book...",composeView,SLOT(toDo()),ALT+Key_B);
  fmenu->insertItem("&Print...",composeView,SLOT(printIt()),ALT+Key_P);
  fmenu->insertSeparator();
  fmenu->insertItem("New &Composer",composeView,SLOT(newComposer()),ALT+Key_C);
  fmenu->insertItem("New Mail&reader",this,SLOT(doNewMailReader()),ALT+Key_R);
  fmenu->insertSeparator();
  fmenu->insertItem("&Close",this,SLOT(abort()),CTRL+ALT+Key_C);
  menuBar->insertItem("File",fmenu);
  

  QPopupMenu  *emenu = new QPopupMenu();
  emenu->insertItem("Undo",composeView,SLOT(undoEvent()));
  emenu->insertSeparator();
  emenu->insertItem("Copy",composeView,SLOT(copyText()),CTRL + Key_C);
  emenu->insertItem("Cut",composeView,SLOT(cutText()),CTRL + Key_X);
  emenu->insertItem("Paste",composeView,SLOT(pasteText()),CTRL + Key_V);
  emenu->insertItem("Mark all",composeView,SLOT(markAll()),CTRL + Key_A);
  emenu->insertSeparator();
  emenu->insertItem("Find...",composeView,SLOT(find()));
  menuBar->insertItem("Edit",emenu);

  QPopupMenu *mmenu = new QPopupMenu();
  mmenu->insertItem("Recip&ients...",composeView,SLOT(toDo()),ALT+Key_I);
  mmenu->insertItem("Insert &File", composeView,SLOT(insertFile()),ALT+Key_F);
  mmenu->insertSeparator();
  QPopupMenu *menv = new QPopupMenu();
  menv->insertItem("High");
  menv->insertItem("Normal");
  menv->insertItem("Low");
  mmenu->insertItem("Priority",menv);
  menuBar->insertItem("Message",mmenu);

  QPopupMenu *amenu = new QPopupMenu();
  amenu->insertItem("&File...",composeView,SLOT(attachFile()),ALT+Key_F);
  amenu->insertItem("Si&gnature",composeView,SLOT(appendSignature()),ALT+Key_G);
  if(sigStatus == true)
  	amenu->setItemEnabled(amenu->idAt(2),FALSE);
  menuBar->insertItem("Attach",amenu);

  QPopupMenu *omenu = new QPopupMenu();
//  omenu->insertItem("General S&ettings",this,SLOT(setSettings()),ALT+Key_E);
  omenu->insertItem("Toggle T&oolbar",this,SLOT(toggleToolBar()),ALT+Key_O);
  omenu->setItemChecked(omenu->idAt(2),TRUE);
  menuBar->insertItem("Options",omenu);

  menuBar->insertSeparator();


  QPopupMenu *hmenu = new QPopupMenu();
  hmenu->insertItem("Help",this,SLOT(invokeHelp()),ALT + Key_H);
  hmenu->insertSeparator();
  hmenu->insertItem("About",this,SLOT(about()));
  menuBar->insertItem("Help",hmenu);

  setMenu(menuBar);
}

void KMComposeWin::setupToolBar()
{
	QString pixdir = "";   // pics dir code "inspired" by kghostview (thanks)
	char *kdedir = getenv("KDEDIR");
	if (kdedir) pixdir.append(kdedir);
	 else pixdir.append("/usr/local/kde");
	pixdir.append("/lib/pics/toolbar/");

	toolBar = new KToolBar(this);

	QPixmap pixmap;

	pixmap.load(pixdir+"kmnew.xpm");
	toolBar->insertButton(pixmap,0,SIGNAL(clicked()),composeView,SLOT(newComposer()),TRUE,"New Composer");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"kmsend.xpm");
	toolBar->insertButton(pixmap,0,SIGNAL(clicked()),composeView,SLOT(sendIt()),TRUE,"Send");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"reload.xpm");
	toolBar->insertButton(pixmap,2,SIGNAL(clicked()),composeView,SLOT(copyText()),TRUE,"Undo");
	pixmap.load(pixdir+"editcopy.xpm");
	toolBar->insertButton(pixmap,3,SIGNAL(clicked()),composeView,SLOT(copyText()),TRUE,"Copy");
	pixmap.load(pixdir+"editcut.xpm");
	toolBar->insertButton(pixmap,4,SIGNAL(clicked()),composeView,SLOT(cutText()),TRUE,"Cut");
	pixmap.load(pixdir+"editpaste.xpm");
	toolBar->insertButton(pixmap,5,SIGNAL(clicked()),composeView,SLOT(pasteText()),TRUE,"Paste");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"thumb_up.xpm");
	toolBar->insertButton(pixmap,6,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Recipients");
	pixmap.load(pixdir+"kmaddressbook.xpm");
	toolBar->insertButton(pixmap,7,SIGNAL(clicked()),composeView,SLOT(toDo()),TRUE,"Addressbook");
	pixmap.load(pixdir+"kmattach.xpm");
	toolBar->insertButton(pixmap,8,SIGNAL(clicked()),composeView,SLOT(attachFile()),TRUE,"Attach");
	toolBar->insertSeparator();
	pixmap.load(pixdir+"kmprint.xpm");
	toolBar->insertButton(pixmap,12,SIGNAL(clicked()),composeView,SLOT(printIt()),TRUE,"Print");
	pixmap.load(pixdir+"help.xpm");
	toolBar->insertButton(pixmap,13,SIGNAL(clicked()),this,SLOT(invokeHelp()),TRUE,"Help");

	addToolBar(toolBar);
}

void KMComposeWin::doNewMailReader()
{
   KMMainWin *newReader = new KMMainWin(NULL);
   newReader->show();
   newReader->resize(newReader->size());
}

void KMComposeWin::toggleToolBar()
{
  enableToolBar(KToolBar::Toggle);
  if(toolBarStatus==false)
	toolBarStatus=true;
  else
	toolBarStatus=false;
  
  repaint();
}

void KMComposeView::appendSignature()
{
   KConfig *configFile = new KConfig();
   QString sigFile;
   char temp[255];
   QString text;
   int col; 
   int line;
   editor->getCursorPosition(&line,&col);
   configFile = KApplication::getKApplication()->getConfig();
   configFile->setGroup("Identity");
   sigFile = configFile->readEntry("Signature File");
   QFile *contFile= new QFile(sigFile);
   contFile->open(IO_ReadOnly);
   while((contFile->readLine(temp,100)) != 0)
        text.append(temp);
   editor->insertAt(text,line,col);
   editor->update();
   editor->repaint();

}



void KMComposeWin::abort()
{
  close();
}

void KMComposeWin::invokeHelp()
{

  KApplication::getKApplication()->invokeHTMLHelp("","");

}

void KMComposeWin::toDo()
{
	//  KMMainWin::doUnimplemented(); //is private :-(

   KMsgBox::message(this,"Ouch",
			 "Not yet implemented!\n"
			 "We are sorry for the inconvenience :-)",1);
}

void KMComposeWin::about()
{
	KMsgBox::message(this,"About",
	 "kmail [ALPHA]\n\n"
	 "Lynx <lynx@topaz.hknet.com>\n"
	 "Stephan Meyer <Stephan.Meyer@munich.netsurf.de>\n"
	 "Stefan Taferner <taferner@alpin.or.at>\n"
	 "Markus Wübben <markus.wuebben@kde.org>\n\n"
	 "This program is covered by the GPL.",1);
}

void KMComposeWin::closeEvent(QCloseEvent *e)
{
	KTopLevelWidget::closeEvent(e);
	delete this;
	KConfig *config = new KConfig();
 	config = KApplication::getKApplication()->getConfig();
	config->setGroup("Settings");
	if(toolBarStatus)
		config->writeEntry("ShowToolBar","yes");
	else
		config->writeEntry("ShowToolBar","no");
	config->sync();

}

KMAttachmentItem::KMAttachmentItem(QString _name, int _index)
{
	
	fileName = _name;
	index =  _index;
	printf("In AttachmentItem constructor\n");
	cout << fileName << "\n";
	cout << index << "\n";
	printf("Leaving item constructor\n");
}
