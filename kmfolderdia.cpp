// kmfolderdia.cpp

#include <qstring.h>
#include <qcstring.h>
#include <qlabel.h>
#include <kapp.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qlayout.h>

#include "kmmainwin.h"
#include "kmglobal.h"
#include "kmfoldermgr.h"

#include <assert.h>
#include <klocale.h>

#include "kmfolderdia.moc"


//-----------------------------------------------------------------------------
KMFolderDialog::KMFolderDialog(const QCString& aPath, const QCString& aName,
			       QWidget *aParent, const QString& aCap):
  KMFolderDialogInherited(KDialogBase::Plain, aCap, 
			  KDialogBase::Ok|KDialogBase::Cancel,
			  KDialogBase::Ok, aParent, 0, true)
{
  QLabel *label;
  QGridLayout* grid;
  QString str;
  int ulx, uly, lrx, lry; 

  mPath = aPath;

  grid = new QGridLayout(this, 3, 2, 20, 6, "grid");

  label = new QLabel(i18n("Path:"), this);
  label->setMinimumSize(label->sizeHint());
  grid->addWidget(label, 0, 0);

  label = new QLabel(aPath, this);
  label->setMinimumSize(label->sizeHint());
  grid->addWidget(label, 0, 1);

  mNameEdit = new QLineEdit(this);
  mNameEdit->setMinimumSize(mNameEdit->sizeHint());
  mNameEdit->setFocus();
  if (aName.isEmpty()) str = i18n("Unnamed");
  else str = aName;
  mNameEdit->setText(str);
  grid->addWidget(mNameEdit, 1, 1);

  label = new QLabel(i18n("Name:"), this);
  label->setMinimumSize(label->sizeHint().width(),mNameEdit->sizeHint().height());
  grid->addWidget(label, 1, 0);

  getBorderWidths(ulx, uly, lrx, lry);
  grid->addRowSpacing(2, lry);

  grid->activate();
}


//-----------------------------------------------------------------------------
QCString KMFolderDialog::folderName() const
{
  return QCString(mNameEdit->text());
}

