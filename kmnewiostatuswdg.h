// KMIOStatusWdg: class to show transmission state
// Author: Markus Wuebben <markus.wuebben@kde.org>
// This code is published under the GPL.

#ifndef KMIOWDG_H
#define KMIOWDG_H

#include <qwidget.h>
#include <qstring.h>
#include <qlabel.h>
#include <qpushbt.h>
#include <kprogress.h>
#include "kmnewiostatus.h"

class KMIOStatusWdg : public KMIOStatus
{
  Q_OBJECT
 public:
  
  KMIOStatusWdg(QWidget *parent = 0, const char * name = 0,
		task = 0, QString host = 0 );
  ~KMIOStatusWdg();


 public slots:

 /**update the ProgressBar. index is current message, message is 
    number of all messages **/
  void updateProgressBar(int index, int messages );

  /** Tell widget that the tranmission has been completed **/
  void transmissionCompleted();

 private slots:
  void abortPressed();
  void update();

 private:

  KProgress *progressBar;
  QPushButton *abortBt;
  QLabel *msgLbl;

};

#endif


