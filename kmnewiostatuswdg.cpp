#include <iostream.h>
#include <kapp.h>
#include "kmnewiostatuswdg.h"
#include "kmnewiostatuswdg.moc"

KMIOStatusWdg::KMIOStatusWdg(QWidget *parent, const char *name, 
			     task type , QString host)
  :KMIOStatus(parent,name) {


  initMetaObject();
  abortPressedBool = false;

  setTask(type);
  setHost(host);

  progressBar = new KProgress(this);
  progressBar->setGeometry(35,45,200,30);

  abortBt = new QPushButton("Abort",this);
  abortBt->resize(abortBt->sizeHint());
  abortBt->move(110,90);
  connect(abortBt,SIGNAL(clicked()),this,SLOT(abortPressed()));

  msgLbl = new QLabel(this);
  msgLbl->setGeometry(43,15,200,25);
  setMaximumSize(270,140);
  setMinimumSize(270,140);

  update();

}

void KMIOStatusWdg::update() {

  if(Task() == SEND)
    setCaption(i18n("Sending messages to ") + host());
  else 
    if(Task() == RETRIEVE)
      setCaption(i18n("Retrieving messages from ") + host());
 
  QWidget::update();

}

void KMIOStatusWdg::prepareTransmission(QString host, task _t) {

  setHost(host);
  setTask(_t);
  msgLbl->setText(i18n("Preparing transmission..."));

}

void KMIOStatusWdg::transmissionCompleted() {

  msgLbl->setText(i18n("Transmission completed..."));

}

KMIOStatusWdg::~KMIOStatusWdg() {

}

void KMIOStatusWdg::newMail(bool /*_newMail*/) {

}

void KMIOStatusWdg::updateProgressBar(int index ,int of) {

  float value;
  QString tmp;

  value = ((float)index/(float)of)*100;
  if(Task() == RETRIEVE)
    tmp.sprintf("Downloading message %i of %i",index,of);
  else
    if(Task() == SEND)
      tmp.sprintf("Sending message %i of %i",index,of);

  msgLbl->setText(tmp);
  msgLbl->resize(msgLbl->sizeHint());
  progressBar->setValue((int)value);
  
}



void KMIOStatusWdg::abortPressed() {

  cout << "Abort requested...\n";
  msgLbl->setText(i18n("Aborting transmission...\n"));
  abortPressedBool = true;
  emit abort();

}







