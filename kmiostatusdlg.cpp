// kmiostatusdlg.cpp
// Author: Stefan Taferner <taferner@kde.org>
// This code is under GPL

#include "kmiostatusdlg.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <klocale.h>
#include <kapp.h>

//-----------------------------------------------------------------------------
KMIOStatusDlg::KMIOStatusDlg(const char* aCap):
  KMIOStatusDlgInherited(NULL, aCap, TRUE)
{
  int y, h, w;

  initMetaObject();

  mLblTask = new QLabel("M", this);
  mLblTask->adjustSize();
  h = mLblTask->size().height();
  w = mLblTask->size().width() * 30;
  y = 5+h;
  mLblTask->setGeometry(10, y, w, h);
  y += y+2;

  mLblStatus = new QLabel(this);
  mLblStatus->setGeometry(10, y, w, h);
  y += y+2;

  mBtnAbort = new QPushButton(i18n("&Abort"), this);
  mBtnAbort->adjustSize();
  mBtnAbort->move(10, y);
  connect(mBtnAbort,SIGNAL(clicked()),this,SLOT(reject()));
}


//-----------------------------------------------------------------------------
void KMIOStatusDlg::done(void)
{
  accept();
}


//-----------------------------------------------------------------------------
void KMIOStatusDlg::setTask(const QString aMsg)
{
  mLblTask->setText(aMsg);
}


//-----------------------------------------------------------------------------
void KMIOStatusDlg::setStatus(const QString aMsg)
{
  mLblStatus->setText(aMsg);
}


//-----------------------------------------------------------------------------
#include "kmiostatusdlg.moc"

