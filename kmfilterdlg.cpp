// kmfilterdlg.cpp

#include <qfiledlg.h>

#include "kmfilterdlg.h"
#include "kmfilter.h"
#include "kmfiltermgr.h"
#include "kmglobal.h"

#include <klocale.h>
#include <kmsgbox.h>
#include <ktablistbox.h>
#include <kbuttonbox.h>
#include <qbttngrp.h>
#include <qframe.h>
#include <qgrpbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qpushbt.h>
#include <qradiobt.h>


//-----------------------------------------------------------------------------
KMFilterDlg::KMFilterDlg(QWidget* parent, const char* name):
  KMFilterDlgInherited(parent, name, FALSE)
{
  QBoxLayout* box;
  KButtonBox* buttonBox;
  QPushButton* btnHelp;

  initMetaObject();

  box  = new QVBoxLayout(this);;
  mBox = new QHBoxLayout;

  setCaption(nls->translate("Filter Rules"));
  //setOKButton(nls->translate("Ok"));
  //setCancelButton(nls->translate("Cancel"));

  mFilterList = new QListBox(this);
  mBox->addWidget(mFilterList);

  mFilterArea = new QWidget(this);
  mBox->addWidget(mFilterArea);

  box->addLayout(mBox);

  buttonBox = new KButtonBox(this);
  box->addWidget(buttonBox);

  mBtnUp = buttonBox->addButton(nls->translate("Up"));
  mBtnDown  = buttonBox->addButton(nls->translate("Down"));
  mBtnNew = buttonBox->addButton(nls->translate("New"));
  mBtnDelete = buttonBox->addButton(nls->translate("Delete"));
  buttonBox->addStretch();
  mBtnOk = buttonBox->addButton(nls->translate("Ok"));
  mBtnCancel = buttonBox->addButton(nls->translate("Cancel"));
  buttonBox->addStretch();
  btnHelp = buttonBox->addButton(nls->translate("Help"));

  box->activate();

  reloadFilterList();
}


//-----------------------------------------------------------------------------
KMFilterDlg::~KMFilterDlg()
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::reloadFilterList(void)
{
  KMFilter* filter;

  mFilterList->clear();

  for(filter=filterMgr->first(); filter; filterMgr->next())
  {
    mFilterList->insertItem(filter->name());
  }
}


//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnUp()
{
}

//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnDown()
{
}

//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnNew()
{
}

//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnDelete()
{
}

//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnOk()
{
  accept();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnCancel()
{
  reject();
}

//-----------------------------------------------------------------------------
void KMFilterDlg::doBtnHelp()
{
}

//-----------------------------------------------------------------------------
void KMFilterDlg::addLabel(const QString aLabel)
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::addEntry(const QString aLabel, QString aValue)
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::addFileNameEntry(const QString aLabel, QString aValue,
		  const QString aPattern, const QString aStartdir)
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::addToggle(const QString aLabel, bool* aValue)
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::addFolderList(const QString aLabel, KMFolder** folderPtr)
{
}


//-----------------------------------------------------------------------------
void KMFilterDlg::addWidget(const QString aLabel, QWidget* aWidget)
{
}


//-----------------------------------------------------------------------------
#include "kmfilterdlg.moc"
