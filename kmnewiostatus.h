// KMIOStatus: base class to show transmission state
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#ifndef KMIO_H
#define KMIO_H

#include <qwidget.h>
#include <qstring.h>
#include <qlabel.h>
#include <qpushbt.h>

class KMIOStatus : public QWidget
{
  Q_OBJECT
 public:
  
  /** Which kind of widget you want **/
  enum task { SEND, RETRIEVE } ;

  KMIOStatus(QWidget *parent = 0, const char *name = 0);
  ~KMIOStatus();


 public slots:

 /** Check if abort was pressed **/  
  bool abortRequested();
 
 /**update the ProgressBar. index is current message, message is 
    number of all messages **/
  virtual void updateProgressBar(int index, int messages );

  /** Set host to download or send to **/
  void setHost(QString);
  
  /** Get host to download from or send to **/
  QString host();

  /** Set widget's task **/
  void setTask(task);

  /** Get widget`s task **/
  task Task();

  /** Tell widget that the tranmission has been completed **/
  virtual void transmissionCompleted();

 private slots:
  virtual void abortPressed();

 /** update the widget as you need to  e.g setCaption **/ 
  virtual void update();

 signals:

 /** Emitted when abort was pressed **/
  void abort();

 protected:

  bool abortPressedBool;
  QString _host;
  task _task;

};

#endif


