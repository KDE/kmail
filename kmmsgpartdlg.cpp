// kmmsgpartdlg.cpp

#include "kmmsgpartdlg.h"
#include "kmmsgpart.h"

#ifndef KRN
#include "kmglobal.h"
#endif

#ifdef KRN
#include <kapp.h>
#include "kbusyptr.h"
extern KLocale *nls;
extern KBusyPtr *kbp;
#endif

#include "kbusyptr.h"
#include <kapp.h>
#include <qcombo.h>
#include <qpushbt.h>
#include <qlabel.h>
#include <qlined.h>
#include <qlayout.h>
#include <unistd.h>
#include <assert.h>


//-----------------------------------------------------------------------------
KMMsgPartDlg::KMMsgPartDlg(const char* aCaption, bool readOnly): 
  KMMsgPartDlgInherited(NULL, "msgpartdlg", TRUE), mIconPixmap()
{
  QGridLayout* grid = new QGridLayout(this, 6, 4, 8, 8);
  QPushButton *btnOk, *btnCancel;
  QLabel *label;
  int h;

  mMsgPart = NULL;

  resize(320, 190);
  if (aCaption) setCaption(aCaption);
  else setCaption(i18n("Message Part Properties"));

  mLblIcon = new QLabel(this);
  mLblIcon->resize(32, 32);
  grid->addMultiCellWidget(mLblIcon, 0, 1, 0, 0);

  //-----
  mEdtMimetype = new QLineEdit(this);
  mEdtMimetype->adjustSize();
  h = mEdtMimetype->sizeHint().height();
  mEdtMimetype->setMinimumSize(100, h);
  mEdtMimetype->setMaximumSize(1024, h);
  grid->addMultiCellWidget(mEdtMimetype, 0, 0, 1, 3);

  //-----
  mLblSize = new QLabel(this);
  mLblSize->adjustSize();
  mLblSize->setMinimumSize(100, h);
  grid->addMultiCellWidget(mLblSize, 1, 1, 1, 3);

  //-----
  label = new QLabel(i18n("Name:"), this);
  label->adjustSize();
  label->setMinimumSize(label->sizeHint().width(), h);
  grid->addWidget(label, 2, 0);

  mEdtName = new QLineEdit(this);
  mEdtName->setMinimumSize(100, h);
  mEdtName->setMaximumSize(1024, h);
  grid->addMultiCellWidget(mEdtName, 2, 2, 1, 3);

  //-----
  label = new QLabel(i18n("Description:"), this);
  label->adjustSize();
  label->setMinimumSize(label->sizeHint().width(), h);
  grid->addWidget(label, 3, 0);

  mEdtComment = new QLineEdit(this);
  mEdtComment->setMinimumSize(100, h);
  mEdtComment->setMaximumSize(1024, h);
  grid->addMultiCellWidget(mEdtComment, 3, 3, 1, 3);

  label = new QLabel(i18n("Encoding:"), this);
  label->adjustSize();
  label->setMinimumSize(label->sizeHint().width(), h);
  grid->addWidget(label, 4, 0);

  mCbxEncoding = new QComboBox(this);
  mCbxEncoding->insertItem(i18n("none (8bit)"));
  mCbxEncoding->insertItem(i18n("base 64"));
  mCbxEncoding->insertItem(i18n("quoted printable"));
  mCbxEncoding->setMinimumSize(100, h);
  mCbxEncoding->setMaximumSize(1024, h);
  grid->addMultiCellWidget(mCbxEncoding, 4, 4, 1, 2);

  if(readOnly)
    {mEdtMimetype->setEnabled(FALSE);
     mEdtName->setEnabled(FALSE);
     mEdtComment->setEnabled(FALSE);
     mCbxEncoding->setEnabled(FALSE);
    }
	

  //-----
  btnOk = new QPushButton(i18n("Ok"), this);
  btnOk->adjustSize();
  btnOk->setMinimumSize(btnOk->sizeHint());
  connect(btnOk, SIGNAL(clicked()), SLOT(accept()));
  grid->addMultiCellWidget(btnOk, 5, 5, 0, 1);

  btnCancel = new QPushButton(i18n("Cancel"), this);
  btnCancel->adjustSize();
  btnCancel->setMinimumSize(btnCancel->sizeHint());
  connect(btnCancel, SIGNAL(clicked()), SLOT(reject()));
  grid->addMultiCellWidget(btnCancel, 5, 5, 2, 3);

  //-----
  grid->setColStretch(0, 0);
  grid->setColStretch(1, 10);
  grid->setColStretch(3, 10);
  grid->activate();

  adjustSize();
}


//-----------------------------------------------------------------------------
KMMsgPartDlg::~KMMsgPartDlg()
{
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::setMsgPart(KMMessagePart* aMsgPart)
{
  unsigned int len, idx;
  QString lenStr(32), iconName, enc;

  mMsgPart = aMsgPart;
  assert(mMsgPart!=NULL);

  mEdtComment->setText(mMsgPart->contentDescription());
  mEdtName->setText(mMsgPart->name());
  mEdtMimetype->setText(mMsgPart->typeStr()+"/"+mMsgPart->subtypeStr());

  len = mMsgPart->body().size()-1;
  if (len > 9999) lenStr.sprintf("%u KB", len>>10);
  else lenStr.sprintf("%u bytes", len);
  mLblSize->setText(lenStr);

  iconName = mMsgPart->iconName();
  mIconPixmap.load(iconName);
  mLblIcon->setPixmap(mIconPixmap);
  mLblIcon->resize(mIconPixmap.size());

  enc = mMsgPart->cteStr();
  if (enc=="base64") idx = 1;
  else if (enc=="quoted printable") idx = 2;
  else idx = 0;
  mCbxEncoding->setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::applyChanges(void)
{
  QString str, body, type, subtype;
  int idx;

  if (!mMsgPart) return;

  kbp->busy();
  str = mEdtName->text();
  if (!str.isEmpty() || !mMsgPart->name().isEmpty())
    mMsgPart->setName(str);

  str = mEdtComment->text();
  if (!str.isEmpty() || !mMsgPart->contentDescription().isEmpty())
    mMsgPart->setContentDescription(str);

  idx = mCbxEncoding->currentItem();
  if (idx==1) str = "base64";
  else if (idx==2) str = "quoted printable";
  else str = "8bit";

  type = mEdtMimetype->text();
  idx = type.find('/');
  if (idx < 0) subtype = "";
  else
  {
    subtype = type.mid(idx+1, 256);
    type = type.left(idx);
  }

  mMsgPart->setTypeStr(type);
  mMsgPart->setSubtypeStr(subtype);

  if (str != mMsgPart->cteStr())
  {
    body = mMsgPart->bodyDecoded();
    mMsgPart->setCteStr(str);
    mMsgPart->setBodyEncoded(body);
  }
  kbp->idle();
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::done(int rc)
{
  if (rc) applyChanges();
  KMMsgPartDlgInherited::done(rc);
}


//-----------------------------------------------------------------------------
#include "kmmsgpartdlg.moc"
