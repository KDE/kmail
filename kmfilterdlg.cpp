// kmfilterdlg.cpp

#include <qfiledlg.h>

#include "kmfilterdlg.h"
#include "kmfilter.h"
#include "kmfiltermgr.h"
#include "kmglobal.h"

#include <klocale.h>
#include <kmsgbox.h>
#include <ktablistbox.h>
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
  QBoxLayout* hbox = new QHBoxLayout(this);

  initMetaObject();

  setCaption(nls->translate("Filter Rules"));
  //setOKButton(nls->translate("Ok"));
  //setCancelButton(nls->translate("Cancel"));

  mFilterList = new QListBox(this, "list");
  hbox->addWidget(mFilterList);

  mBox = new QVBoxLayout;

  

  //hbox->addLayout(mBox);

  hbox->activate();
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
