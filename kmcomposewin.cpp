// kmcomposewin.cpp

#include <klocale.h>
#include <unistd.h>
#include <qfiledlg.h>
#include <qaccel.h>
#include <qlabel.h>
#include "kmcomposewin.moc"
#include "kmmainwin.h"
#include "kmmessage.h"
#include "kmglobal.h"
#include "kmsender.h"
#include <iostream.h>
#include <qwidget.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mimelib/string.h>
#include <kiconloader.h>

//-----------------------------------------------------------------------------
KMComposeView::KMComposeView(QWidget *parent, const char *name, 
			     QString emailAddress, KMMessage *message, 
			     int action): 
  QWidget(parent, name)
{
  QLabel* label;
  QSize sz;

  attWidget = NULL;
  if (message) currentMessage = message;
  else currentMessage = new KMMessage(); 
  indexAttachment =0;

  grid = new QGridLayout(this,10,2,2,4);	

  toLEdit = new QLineEdit(this);
  label = new QLabel(toLEdit, nls->translate("&To:"), this);
  label->adjustSize();
  label->setMinimumSize(label->size());
  grid->addWidget(label,0,0);

  sz.setWidth(100);
  sz.setHeight(label->size().height() + 6);

  toLEdit->setMinimumSize(sz);
  grid->addWidget(toLEdit,0,1);

  ccLEdit = new QLineEdit(this);
  label = new QLabel(ccLEdit, nls->translate("&Cc:"), this);
  label->adjustSize();
  label->setMinimumSize(label->size());
  grid->addWidget(label,1,0);

  subjLEdit = new QLineEdit(this);
  label = new QLabel(subjLEdit, nls->translate("&Subject:"), this);
  label->adjustSize();
  label->setMinimumSize(label->size());
  grid->addWidget(label,2,0);
  connect(subjLEdit,SIGNAL(textChanged(const char *)),
	  SLOT(slotChangeHeading(const char *)));

  if (emailAddress) 
    toLEdit->setText(emailAddress);
  ccLEdit->setMinimumSize(sz);
  grid->addWidget(ccLEdit,1,1);

  subjLEdit->setMinimumSize(sz);
  grid->addWidget(subjLEdit,2,1);
  
  editor = new KEdit(0,this);
  grid->addMultiCellWidget(editor,3,8,0,1);
  grid->setRowStretch(3,100);

  attachmentWidget = new KTabListBox(this,NULL,3);
  attachmentWidget->setColumn(0,"F",20,KTabListBox::PixmapColumn);
  attachmentWidget->setColumn(1,"Filename",parent->width()-120);
  attachmentWidget->setColumn(2,"Size",60);
  grid->addMultiCellWidget(attachmentWidget,9,9,0,1);
  attachmentWidget->hide();  

  zone = new KDNDDropZone(editor,DndURL);
  connect(zone,SIGNAL(dropAction(KDNDDropZone *)),SLOT(getDNDAttachment()));

  urlList = new QStrList;
	
  grid->setColStretch(1,100);

  if(message && action==FORWARD) forwardMessage();	
  else if(message && action==REPLY) replyMessage();
  else if(message && action ==REPLYALL) replyAll();

  grid->activate();

  parseConfiguration();	
}


//-----------------------------------------------------------------------------
KMComposeView::~KMComposeView()
{}

//-----------------------------------------------------------------------------
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
    text += nls->translate("To:");
    text += " \n";
    text += toLEdit->text();
    text += "\n";
    text += nls->translate("Subject:");
    text += " \n";
    text += subjLEdit->text();
    text += nls->translate("Date:");
    text += " \n\n";
    text += editor->text();
    text.replace(QRegExp("\n"),"\n");
    paint.drawText(30,30,text);
    paint.end();
  }
  delete printer;
}

//-----------------------------------------------------------------------------
void KMComposeView::attachFile()
{
  QString atmntFile;
  QString fileName;

  QFileDialog *d=new QFileDialog(".","*",this,NULL,TRUE);
  d->setCaption(nls->translate("Attach File"));
  if (d->exec()) 
    atmntFile = d->selectedFile();		
  delete d;
  urlList->append(atmntFile);
  if(urlList->count() == 1)// Create attachmentWidget
    {createAttachmentWidget();
    insertNewAttachment(atmntFile);}
  else                     // Add new icon to attachmentWidget
    insertNewAttachment(atmntFile);
}

//-----------------------------------------------------------------------------
void KMComposeView::getDNDAttachment()
{
  QString element;
  QStrList *tempList = new QStrList(); 
  *tempList= zone->getURLList();
  element = tempList->first();
  if(element.find("file:",0,0) >= 0)
    element.replace("file:","");
  urlList->append(element);
  element = urlList->first();
  cout << "Elements in the list: " << urlList->count() << "\n";
  while(element.isEmpty() == FALSE)
    {cout << element << "\n";
    element = urlList->next();}

  if(urlList->count() == 1)
    {createAttachmentWidget();
    insertNewAttachment(tempList->first());}
  else
    insertNewAttachment(tempList->first());
  delete tempList;       
}

//-------------------------------------------------------------------------
void KMComposeView::createAttachmentWidget()
{
  cout << "Making attachmentWidget visible\n" ;
  attachmentWidget->show();
  grid->setRowStretch(3,3);
  grid->setRowStretch(9,1);  
  
}
   
//--------------------------------------------------------------------------
void KMComposeView::deleteAttachmentWidget()
{
}

//--------------------------------------------------------------------------

void KMComposeView::insertNewAttachment(QString File)
{
  QString element;
  cout << "Inserting Attachment\"" + File << "\" into widget\n";
  element = urlList->first();
  cout << "Elements to be inserted: " << urlList->count() << "\n";
  attachmentWidget->clear();
  while(element.isEmpty() == FALSE)
    {cout << element << "\n";
    element = " \n" + element + "\n ";
    attachmentWidget->insertItem(element);
    element = urlList->next();}

  resize(this->size());
  
}

//-----------------------------------------------------------------------------
KMMessage * KMComposeView::prepareMessage()
{
  QString option;
  KMMessage *msg = new KMMessage();
  msg = currentMessage;

  QString temp=toLEdit->text();
  if (temp.isEmpty()) {
    warning(nls->translate("No recipients defined."));
    msg = 0;
    return msg;}

  // Now, I have a problems with the CRLF. Everything works fine under 
  // Unix (of course ;-) ) but under MS-Windowz the CRLF is not inter-
  // preted. Why??

  temp = editor->text();
  temp.replace(QRegExp("\r"),"\r\n");
	
  // The the necessary Message() stuff

  msg->setFrom(EMailAddress);
  msg->setReplyTo(ReplyToAddress);
  msg->setTo(toLEdit->text());
  msg->setCc(ccLEdit->text());
  msg->setSubject(subjLEdit->text());
  if(urlList->count() == 0)
    msg->setBody(temp); // If there are no attachments
  else
    {//	create bodyPart for editor text.
     KMMessagePart *part = new KMMessagePart();
     part->setCteStr("7-bit"); 
     part->setTypeStr("Text");
     part->setSubtypeStr("Plain");
     part->setBody(temp);
     msg->addBodyPart(part);

     // Since there is at least one more attachment create another bodypart
     QString atmntStr;
     atmntStr = urlList->first();
     part = new KMMessagePart();
     if((part = createKMMsgPart(part, atmntStr)) !=0)
       msg->addBodyPart(part);
	
     // As long as there are more attachments in the queue let's add bodyParts
     while((atmntStr = urlList->next()) != 0)
       {part = new KMMessagePart();
       if((part = createKMMsgPart(part,atmntStr)) != 0)
	 {msg->addBodyPart(part);
	 part = new KMMessagePart();}
       else
	 {printf("no msgPart\n"); // Probably could not open file
	 part = new KMMessagePart();}
       }
    }

  msg->setAutomaticFields();
  return msg;
}

KMMessagePart * KMComposeView::createKMMsgPart(KMMessagePart *p, QString s)
{
  printf("Creating MessagePart\n");
  QFile *file = new QFile(s);
  QString str;
  char buf[255];

  p->setCteStr("BASE64");
  if(!file->open(IO_ReadOnly))
    {KMsgBox::message(0,"Error","Can't open attachment file");
    p=0;
    return p;}

  file->at(0);
  while(file->readLine(buf,255) != 0)
    str.append(buf);
  file->close();

  p->setBody(str);
  printf("Leaving MessagePart....\n");
  return p;
  
}



//----------------------------------------------------------------------------

void KMComposeView::sendNow()
{
  KMMessage *msg = new KMMessage();
  if((msg = prepareMessage()) == 0)
    return;
  msgSender->send(msg);
  ((KMComposeWin *)parentWidget())->close();
}

//----------------------------------------------------------------------------
void KMComposeView::sendLater()
{
  KMMessage *msg = new KMMessage();
  if((msg =prepareMessage()) == 0)
    return;
  msgSender->send(msg,FALSE);
  ((KMComposeWin *)parentWidget())->close();
}

  


void KMComposeView::slotPopupMenu(const char *_url, const QPoint &point)
{
  cout << "_url: " << _url << "\n";
  QPopupMenu *p = new QPopupMenu();
  p->insertItem("Open");
  p->insertItem("Save as...");
  p->insertItem("Delete");
  p->insertItem("Properties");
  p->popup(point);
}
//-----------------------------------------------------------------------------

void KMComposeView::parseConfiguration()
{
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
  ReplyToAddress = config->readEntry("Reply-To Address");
  cout << ReplyToAddress << "\n";
  

}

//-----------------------------------------------------------------------------
void KMComposeView::forwardMessage()
{
  QString temp, spc;
  temp.sprintf(nls->translate("Fwd: %s"),currentMessage->subject());
  subjLEdit->setText(temp);

  spc = " ";

  editor->append(QString("\n\n---------")+nls->translate("Forwarded message")
		 +"-----------");
  editor->append(nls->translate("Date:") + spc + currentMessage->dateStr());
  editor->append(nls->translate("From:") + spc + currentMessage->from());
  editor->append(nls->translate("To:") + spc + currentMessage->to());
  editor->append(nls->translate("Cc:") + spc + currentMessage->cc());
  editor->append(nls->translate("Subject:") + spc + currentMessage->subject());

  editor->append(temp);
  if ((currentMessage->numBodyParts()) == 0) 
    temp = currentMessage->body();
  else
    {KMMessagePart *p = new KMMessagePart();
    currentMessage->bodyPart(0,p);
    temp = p->body();
    delete p;}
  editor->append(temp);

  editor->update();
  editor->repaint();
	
}

//-----------------------------------------------------------------------------
void KMComposeView::replyAll()
{
  QString temp;
  int lines;
  temp.sprintf(nls->translate("Re: %s"),currentMessage->subject());
  toLEdit->setText(currentMessage->from());
  ccLEdit->setText(currentMessage->cc());
  subjLEdit->setText(temp);

  temp.sprintf(nls->translate("\nOn %s %s wrote:\n"), 
	       currentMessage->dateStr(), currentMessage->from());
  editor->append(temp);

  if ((currentMessage->numBodyParts()) == 0) 
    temp = currentMessage->body();
  else
    {KMMessagePart *p = new KMMessagePart();
    currentMessage->bodyPart(0,p);
    temp = p->body();
    delete p;}  

  editor->append(temp);

  lines = editor->numLines();
  for(int x=2;x < lines;x++)
    editor->insertAt("> ",x,0);

  editor->update();
	  
  currentMessage = currentMessage->reply();
}

//-----------------------------------------------------------------------------
void KMComposeView::replyMessage()
{
  QString temp;
  int lines;

  temp.sprintf(nls->translate("Re: %s"),currentMessage->subject());
  toLEdit->setText(currentMessage->from());
  subjLEdit->setText(temp);

  temp.sprintf(nls->translate("\nOn %s %s wrote:\n"), 
	       currentMessage->dateStr(), currentMessage->from());
  editor->append(temp);

  if ((currentMessage->numBodyParts()) == 0) 
    temp = currentMessage->body();
  else
    {KMMessagePart *p = new KMMessagePart();
    currentMessage->bodyPart(0,p);
    temp = p->body();
    delete p;}
   
    
  editor->append(temp);

  lines = editor->numLines();
  for(int x=2;x < lines;x++)
    {editor->insertAt("> ",x,0);
    }
  editor->update();
  currentMessage = currentMessage->reply();
}

//-----------------------------------------------------------------------------
void KMComposeView::undoEvent()
{
}

//-----------------------------------------------------------------------------
void KMComposeView::copyText()
{
  editor->copyText();
}

//-----------------------------------------------------------------------------
void KMComposeView::cutText()
{
  editor->cut();
}

//-----------------------------------------------------------------------------
void KMComposeView::pasteText()
{
  editor->paste();
}

//-----------------------------------------------------------------------------
void KMComposeView::markAll()
{
  editor->selectAll();
}

//-----------------------------------------------------------------------------
void KMComposeView::find()
{
  editor->Search();
}

//-----------------------------------------------------------------------------
void KMComposeView::detachFile(int , int)
{
}

void KMComposeView::slotSelectFont()
{
  editor->selectFont();
}

//-----------------------------------------------------------------------------
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

void KMComposeView::slotChangeHeading(const char *text)
{
  ((KMComposeWin*)parentWidget())->setCaption(text);
}

//-----------------------------------------------------------------------------
void KMComposeView::toDo()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}


//-----------------------------------------------------------------------------
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
  if(!contFile->open(IO_ReadOnly))
    {KMsgBox::message(0,"Error","Could not open Signature file!");
    return;}
  while((contFile->readLine(temp,100)) != 0)
    text.append(temp);
  contFile->close();
  editor->insertAt(text,line,col);
  editor->update();
  editor->repaint();

}


//-----------------------------------------------------------------------------
void KMComposeView::newComposer()
{
  KMComposeWin *newComposer = new KMComposeWin(NULL,NULL,NULL,NULL);
  newComposer->show();
  newComposer->resize(newComposer->size());

}

void KMComposeView::slotEncodingChanged()
{
  // This slot is called if the Encoding in the menuBar was changed
  toDo();
}


//-----------------------------------------------------------------------------
KMComposeWin::KMComposeWin(QWidget *, const char *name, QString emailAddress,
			   KMMessage *message, int action) : 
  KTopLevelWidget(name)
{
  setCaption(nls->translate("KMail Composer"));

  parseConfiguration();

  composeView = new KMComposeView(this,NULL,emailAddress,message,action);
  setView(composeView,FALSE);

  setupMenuBar();
  setupToolBar();
  setupStatusBar();

  if(toolBarStatus==false)
    enableToolBar(KToolBar::Hide);	
	
  resize(480, 510);
}

//-----------------------------------------------------------------------------
void KMComposeWin::show()
{
  KTopLevelWidget::show();
  resize(size());
}

//-----------------------------------------------------------------------------
void KMComposeWin::parseConfiguration()
{
  KConfig *config;
  QString o;

  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  o = config->readEntry("ShowToolBar");
  if((!o.isEmpty() && o.find("no",0,false)) == 0)
    toolBarStatus = false; 
  else
    toolBarStatus = true;
    

  o = config->readEntry("Signature");
  if((!o.isEmpty() && o.find("auto",0,false)) == 0)
    sigStatus = false;   
  else
    sigStatus = true;
    

  o = config->readEntry("Send button");
  if((!o.isEmpty() && o.find("later",0,false)) == 0)
    sendButton = false;
  else
    sendButton = true;
}	


//-----------------------------------------------------------------------------
void KMComposeWin::setupMenuBar()
{
  menuBar = new KMenuBar(this);

  QPopupMenu *fmenu = new QPopupMenu();
  fmenu->insertItem(nls->translate("Send"),composeView,
		    SLOT(sendNow()), ALT+Key_X);
  fmenu->insertItem(nls->translate("Send &later"),composeView,
		    SLOT(sendLater()),ALT+Key_L);
  fmenu->insertSeparator();
  fmenu->insertItem(nls->translate("Address &Book..."),composeView,
		    SLOT(toDo()),ALT+Key_B);
  fmenu->insertItem(nls->translate("&Print..."),composeView,
		    SLOT(printIt()),ALT+Key_P);
  fmenu->insertSeparator();
  fmenu->insertItem(nls->translate("New &Composer"),composeView,
		    SLOT(newComposer()),ALT+Key_C);
  fmenu->insertItem(nls->translate("New Mail&reader"),this,
		    SLOT(doNewMailReader()),ALT+Key_R);
  fmenu->insertSeparator();
  fmenu->insertItem(nls->translate("&Close"),this,
		    SLOT(abort()),CTRL+ALT+Key_C);
  menuBar->insertItem(nls->translate("File"),fmenu);
  

  QPopupMenu  *emenu = new QPopupMenu();
  emenu->insertItem(nls->translate("Undo"),composeView,
		    SLOT(undoEvent()));
  emenu->insertSeparator();
  emenu->insertItem(nls->translate("Cut"),composeView,
		    SLOT(cutText()),CTRL + Key_X);
  emenu->insertItem(nls->translate("Copy"),composeView,
		    SLOT(copyText()),CTRL + Key_C);
  emenu->insertItem(nls->translate("Paste"),composeView,
		    SLOT(pasteText()),CTRL + Key_V);
  emenu->insertItem(nls->translate("Mark all"),composeView,
		    SLOT(markAll()),CTRL + Key_A);
  emenu->insertSeparator();
  emenu->insertItem(nls->translate("Find..."),composeView,
		    SLOT(find()));
  menuBar->insertItem(nls->translate("Edit"),emenu);

  QPopupMenu *mmenu = new QPopupMenu();
  mmenu->insertItem(nls->translate("Recip&ients..."),composeView,
		    SLOT(toDo()),ALT+Key_I);
  mmenu->insertItem(nls->translate("Insert &File"), composeView,
		    SLOT(insertFile()),ALT+Key_F);
  mmenu->insertSeparator();

  QPopupMenu *menv = new QPopupMenu();
  menv->insertItem(nls->translate("High"));
  menv->insertItem(nls->translate("Normal"));
  menv->insertItem(nls->translate("Low"));
  mmenu->insertItem(nls->translate("Priority"),menv);
  menuBar->insertItem(nls->translate("Message"),mmenu);

  QPopupMenu *amenu = new QPopupMenu();
  amenu->insertItem(nls->translate("&File..."),composeView,
		    SLOT(attachFile()),ALT+Key_F);
  amenu->insertItem(nls->translate("Si&gnature"),composeView,
		    SLOT(appendSignature()),ALT+Key_G);

  mmenu = new QPopupMenu();
  mmenu->setCheckable(TRUE);
  mmenu->insertItem(nls->translate("Base 64"),composeView,
		    SLOT(slotEncodingChanged()));
  mmenu->insertItem(nls->translate("Quoted Printable"),composeView,
		    SLOT(slotEncodingChanged()));
  mmenu->setItemChecked(mmenu->idAt(0),TRUE);
  amenu->insertItem(nls->translate("Encoding"),mmenu);
  menuBar->insertItem(nls->translate("Attach"),amenu);

  QPopupMenu *omenu = new QPopupMenu();
  omenu->insertItem(nls->translate("Toggle T&oolbar"),this,
		    SLOT(toggleToolBar()),ALT+Key_O);
  omenu->insertItem(nls->translate("Change &Font"),composeView,
		    SLOT(slotSelectFont()),ALT+Key_F);
  omenu->setItemChecked(omenu->idAt(2),TRUE);
  menuBar->insertItem(nls->translate("Options"),omenu);
  menuBar->insertSeparator();

  QPopupMenu *hmenu = new QPopupMenu();
  hmenu->insertItem(nls->translate("Help"),this,SLOT(invokeHelp()),ALT + Key_H);
  hmenu->insertSeparator();
  hmenu->insertItem(nls->translate("About"),this,SLOT(about()));
  menuBar->insertItem(nls->translate("Help"),hmenu);

  setMenu(menuBar);
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupToolBar()
{
  KIconLoader* loader = kapp->getIconLoader();

  toolBar = new KToolBar(this);

  toolBar->insertButton(loader->loadIcon("filenew.xpm"),0,
			SIGNAL(clicked()),composeView,
			SLOT(newComposer()),TRUE,"Compose new message");
  toolBar->insertSeparator();

  toolBar->insertButton(loader->loadIcon("toolbar/send.xpm"),0,
			SIGNAL(clicked()),this,
			SLOT(send()),TRUE,"Send message");
  toolBar->insertSeparator();
  toolBar->insertButton(loader->loadIcon("reload.xpm"),2,
			SIGNAL(clicked()),composeView,
			SLOT(copyText()),TRUE,"Undo last change");
  toolBar->insertButton(loader->loadIcon("editcopy.xpm"),3,
			SIGNAL(clicked()),composeView,
			SLOT(copyText()),TRUE,"Copy selection");
  toolBar->insertButton(loader->loadIcon("editcut.xpm"),4,
			SIGNAL(clicked()),composeView,
			SLOT(cutText()),TRUE,"Cut selection");
  toolBar->insertButton(loader->loadIcon("editpaste.xpm"),5,
			SIGNAL(clicked()),composeView,
			SLOT(pasteText()),TRUE,"Paste selection");
  toolBar->insertSeparator();
  toolBar->insertButton(loader->loadIcon("thumb_up.xpm"),6,
			SIGNAL(clicked()),composeView,
			SLOT(toDo()),TRUE,"Recipients");
  toolBar->insertButton(loader->loadIcon("OpenBook.xpm"),7,
			SIGNAL(clicked()),composeView,
			SLOT(toDo()),TRUE,"Open addressbook");
  toolBar->insertButton(loader->loadIcon("attach.xpm"),8,
			SIGNAL(clicked()),composeView,
			SLOT(attachFile()),TRUE,"Attach file");
  toolBar->insertSeparator();
  toolBar->insertButton(loader->loadIcon("fileprint.xpm"),12,
			SIGNAL(clicked()),composeView,
			SLOT(printIt()),TRUE,"Print message");
  toolBar->insertButton(loader->loadIcon("help.xpm"),13,
			SIGNAL(clicked()),this,
			SLOT(invokeHelp()),TRUE,"Help");

  addToolBar(toolBar);
}

//-----------------------------------------------------------------------------
void KMComposeWin::setupStatusBar()
{
  statusBar = new KStatusBar(this);
  statusBar->insertItem("Status:",0);
  setStatusBar(statusBar);
}

//-----------------------------------------------------------------------------
void KMComposeWin::doNewMailReader()
{
  KMMainWin *newReader = new KMMainWin(NULL);
  newReader->show();
  newReader->resize(newReader->size());
}

//-----------------------------------------------------------------------------
void KMComposeWin::toggleToolBar()
{
  enableToolBar(KToolBar::Toggle);
  if(toolBarStatus==false)
    toolBarStatus=true;
  else
    toolBarStatus=false;

  repaint();
}


//-----------------------------------------------------------------------------
void KMComposeWin::abort()
{
  close();
}
void KMComposeWin::about()
{  
  KMsgBox::message(this,nls->translate("About"),
		   "KMail [V0.1] by\n\n"
		   "Stefan Taferner <taferner@alpin.or.at>,\n"
		   "Markus Wübben <markus.wuebben@kde.org>\n\n" 
		   "based on the work of:\n"
		   "Lynx <lynx@topaz.hknet.com>,\n"
		   "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
		   "and the above authors.\n"
		   "This program is covered by the GPL.",1);
}

//-----------------------------------------------------------------------------
void KMComposeWin::invokeHelp()
{

  KApplication::getKApplication()->invokeHTMLHelp("","");
}

void KMComposeWin::send()
{
  if(!sendButton)
    composeView->sendNow();
  else
    composeView->sendLater();
}

//-----------------------------------------------------------------------------
void KMComposeWin::toDo()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}

//-----------------------------------------------------------------------------
void KMComposeWin::closeEvent(QCloseEvent *e)
{
  KTopLevelWidget::closeEvent(e);
  KConfig *config = new KConfig();
  config = KApplication::getKApplication()->getConfig();
  config->setGroup("Settings");
  fflush(stdout);
  config->writeEntry("ShowToolBar", toolBarStatus ? "yes" : "no");
  config->sync();
  delete this;
}












