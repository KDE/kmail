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

  subjLEdit = new QLineEdit(this);
  label = new QLabel(subjLEdit, nls->translate("&Cc:"), this);
  label->adjustSize();
  label->setMinimumSize(label->size());
  grid->addWidget(label,1,0);

  ccLEdit = new QLineEdit(this);
  label = new QLabel(ccLEdit, nls->translate("&Subject:"), this);
  label->adjustSize();
  label->setMinimumSize(label->size());
  grid->addWidget(label,2,0);

  if (emailAddress) 
    toLEdit->setText(emailAddress);
  ccLEdit->setMinimumSize(sz);
  grid->addWidget(ccLEdit,1,1);

  subjLEdit->setMinimumSize(sz);
  grid->addWidget(subjLEdit,2,1);

  editor = new KEdit(0,this);
  grid->addMultiCellWidget(editor,3,9,0,1);
  grid->setRowStretch(3,100);
	
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
void KMComposeView::getDNDAttachment()
{
  QString element;
  QStrList *tempList = new QStrList(); 
  *tempList= zone->getURLList();
  urlList->append(tempList->first());
  element = urlList->first();
  cout << "Elements in the list: " << urlList->count() << "\n";
  while(element.isEmpty() == FALSE)
    {cout << element << "\n";
    element = urlList->next();}
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
    //text.sprintf("From: %s \n",EMailAddress); 
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
}

//-----------------------------------------------------------------------------
void KMComposeView::sendIt()
{
  QString option;

  // Now, what are we going to do: sendNow() or sendLater()?

  KMMessage *msg = new KMMessage();
  msg = currentMessage;

  // Now all items in the attachment queue are being displayed.

  QString temp=toLEdit->text();
  if (temp.isEmpty()) {
    warning(nls->translate("No recipients defined."));
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

  // Get attachments from the list 
  if(urlList->first != 0)
	{QString atmntStr;
	atmntStr = urlList->first();
	KMMessagePart *part = new KMMessagePart();
	part = createKMMsgPart(part, atmntStr);
	msg->addBodyPart(part);
	while((atmntStr =urlList->next()) != 0)
		{part = new KMMessagePart();
		part = createKMMsgPart(part,atmntStr);
		msg->addBodyPart(part);
		delete part;}
		}

  msg->setFrom(EMailAddress);
  msg->setTo(toLEdit->text());
  msg->setCc(ccLEdit->text());
  msg->setSubject(subjLEdit->text());
  msg->setBody(temp);

  // If sending fails the message is queued into the outbox and
  // is sent later. Also the sender takes care of error messages.
  msgSender->send(msg); 


  ((KMComposeWin *)parentWidget())->close();
}

KMMessagePart * KMComposeView::createKMMsgPart(KMMessagePart *p, QString str)
{

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
}

//-----------------------------------------------------------------------------
void KMComposeView::forwardMessage()
{
  QString temp, spc;
  long length;
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

  temp = currentMessage->body(&length);
  if (currentMessage->numBodyParts() > 1) temp.truncate(length);
  editor->append(temp);

  editor->update();
  editor->repaint();
	
}

//-----------------------------------------------------------------------------
void KMComposeView::replyAll()
{
  QString temp;
  long length;
  int lines;
  temp.sprintf(nls->translate("Re: %s"),currentMessage->subject());
  toLEdit->setText(currentMessage->from());
  ccLEdit->setText(currentMessage->cc());
  subjLEdit->setText(temp);

  temp.sprintf(nls->translate("\nOn %s %s wrote:\n"), 
	       currentMessage->dateStr(), currentMessage->from());
  editor->append(temp);

  temp = currentMessage->body(&length);
  if (currentMessage->numBodyParts() > 1) temp.truncate(length);
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
  long length;
  int lines;

  temp.sprintf(nls->translate("Re: %s"),currentMessage->subject());
  toLEdit->setText(currentMessage->from());
  subjLEdit->setText(temp);

  temp.sprintf(nls->translate("\nOn %s %s wrote:\n"), 
	       currentMessage->dateStr(), currentMessage->from());
  editor->append(temp);

  temp = currentMessage->body(&length);
  if (currentMessage->numBodyParts() > 1) temp.truncate(length);
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

//-----------------------------------------------------------------------------
void KMComposeView::toDo()
{
  warning(nls->translate("Sorry, but this feature\nis still missing"));
}

//-----------------------------------------------------------------------------
void KMComposeView::newComposer()
{
  KMComposeWin *newComposer = new KMComposeWin(NULL,NULL,NULL,NULL);
  newComposer->show();
  newComposer->resize(newComposer->size());

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
    {toolBarStatus = false;
    }
  else
    {toolBarStatus = true;
    }

  o = config->readEntry("Signature");
  if((!o.isEmpty() && o.find("auto",0,false)) == 0)
    {sigStatus = false;
    }
  else
    {sigStatus = true;
    }

  o = config->readEntry("Send button");
  if((!o.isEmpty() && o.find("later",0,false)) == 0)
    {sendButton = false;
    }
  else
    sendButton = true;
}	


//-----------------------------------------------------------------------------
void KMComposeWin::setupMenuBar()
{
  menuBar = new KMenuBar(this);

  QPopupMenu *fmenu = new QPopupMenu();
  fmenu->insertItem(nls->translate("Send"),composeView,SLOT(sendIt()), ALT+Key_X);
  fmenu->insertItem(nls->translate("Send &later"),composeView,SLOT(toDo()),ALT+Key_L);
  fmenu->insertSeparator();
  fmenu->insertItem(nls->translate("Address &Book..."),composeView,SLOT(toDo()),ALT+Key_B);
  fmenu->insertItem(nls->translate("&Print..."),composeView,SLOT(printIt()),ALT+Key_P);
  fmenu->insertSeparator();
  fmenu->insertItem(nls->translate("New &Composer"),composeView,SLOT(newComposer()),ALT+Key_C);
  fmenu->insertItem(nls->translate("New Mail&reader"),this,SLOT(doNewMailReader()),ALT+Key_R);
  fmenu->insertSeparator();
  fmenu->insertItem(nls->translate("&Close"),this,SLOT(abort()),CTRL+ALT+Key_C);
  menuBar->insertItem(nls->translate("File"),fmenu);
  

  QPopupMenu  *emenu = new QPopupMenu();
  emenu->insertItem(nls->translate("Undo"),composeView,SLOT(undoEvent()));
  emenu->insertSeparator();
  emenu->insertItem(nls->translate("Cut"),composeView,SLOT(cutText()),CTRL + Key_X);
  emenu->insertItem(nls->translate("Copy"),composeView,SLOT(copyText()),CTRL + Key_C);
  emenu->insertItem(nls->translate("Paste"),composeView,SLOT(pasteText()),CTRL + Key_V);
  emenu->insertItem(nls->translate("Mark all"),composeView,SLOT(markAll()),CTRL + Key_A);
  emenu->insertSeparator();
  emenu->insertItem(nls->translate("Find..."),composeView,SLOT(find()));
  menuBar->insertItem(nls->translate("Edit"),emenu);

  QPopupMenu *mmenu = new QPopupMenu();
  mmenu->insertItem(nls->translate("Recip&ients..."),composeView,SLOT(toDo()),ALT+Key_I);
  mmenu->insertItem(nls->translate("Insert &File"), composeView,SLOT(insertFile()),ALT+Key_F);
  mmenu->insertSeparator();
  QPopupMenu *menv = new QPopupMenu();
  menv->insertItem(nls->translate("High"));
  menv->insertItem(nls->translate("Normal"));
  menv->insertItem(nls->translate("Low"));
  mmenu->insertItem(nls->translate("Priority"),menv);
  menuBar->insertItem(nls->translate("Message"),mmenu);

  QPopupMenu *amenu = new QPopupMenu();
  amenu->insertItem(nls->translate("&File..."),composeView,SLOT(attachFile()),ALT+Key_F);
  amenu->insertItem(nls->translate("Si&gnature"),composeView,SLOT(appendSignature()),ALT+Key_G);
  if(sigStatus == true)
    amenu->setItemEnabled(amenu->idAt(2),FALSE);
  menuBar->insertItem(nls->translate("Attach"),amenu);

  QPopupMenu *omenu = new QPopupMenu();
  //  omenu->insertItem(nls->translate("General S&ettings"),this,SLOT(setSettings()),ALT+Key_E);
  omenu->insertItem(nls->translate("Toggle T&oolbar"),this,SLOT(toggleToolBar()),ALT+Key_O);
  omenu->setItemChecked(omenu->idAt(2),TRUE);
  menuBar->insertItem(nls->translate("Options"),omenu);
  menuBar->insertSeparator();

#ifdef REMOVED
  QPopupMenu *hmenu = new QPopupMenu();
  hmenu->insertItem(nls->translate("Help"),this,SLOT(invokeHelp()),ALT + Key_H);
  hmenu->insertSeparator();
  hmenu->insertItem(nls->translate("About"),parent(),SLOT(about()));
  menuBar->insertItem(nls->translate("Help"),hmenu);
#endif

  setMenu(menuBar);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void KMComposeWin::abort()
{
  close();
}

//-----------------------------------------------------------------------------
void KMComposeWin::invokeHelp()
{

  KApplication::getKApplication()->invokeHTMLHelp("","");
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

//-----------------------------------------------------------------------------
KMAttachmentItem::KMAttachmentItem(QString _name, int _index)
{
	
  fileName = _name;
  index =  _index;
  cout << fileName << "\n";
  cout << index << "\n";
}












