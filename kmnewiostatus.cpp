// KMIOStatus: base class impl. to show transmission state
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#include <iostream.h>
#include "kmnewiostatus.h"
#include "kmnewiostatus.moc"

KMIOStatus::KMIOStatus(QWidget *parent, const char *name) 
  :QWidget(parent, name) {

  initMetaObject();

}

void KMIOStatus::setHost(QString host) {

  _host = host;
  update();

}

QString KMIOStatus::host() {
  
  return _host;
  
}

void KMIOStatus::setTask(task type) {

  _task = type;
  update(); // new task new text. Specified in update()

}

// Reimplement for your impl.
void KMIOStatus::update() {

}

void KMIOStatus::newMail(bool /*_newMail*/) {

}

KMIOStatus::task KMIOStatus::Task() {

  return _task;

}


void KMIOStatus::prepareTransmission(QString,task) {
  update();
  
}

void KMIOStatus::transmissionCompleted() {

 
}

KMIOStatus::~KMIOStatus() {

}

void KMIOStatus::updateProgressBar(int,int) {

}

bool KMIOStatus::abortRequested() {

  return abortPressedBool;

}


void KMIOStatus::abortPressed() {

  cout << "Abort requested...\n";
  abortPressedBool = true;
  emit abort();

}











