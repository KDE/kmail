// kmmsgpartdlg.cpp

#include "kmmsgpartdlg.h"
#include "kmmsgpart.h"
#include "kmglobal.h"
#include "kbusyptr.h"
#include <klocale.h>
#include <qcombo.h>
#include <qpushbt.h>
#include <qlabel.h>
#include <qlined.h>
#include <qlayout.h>
#include <unistd.h>
#include <assert.h>


//-----------------------------------------------------------------------------
KMMsgPartDlg::KMMsgPartDlg(const char* aCaption): 
  KMMsgPartDlgInherited(NULL, "msgpartdlg", TRUE), mIconPixmap()
{
  QGridLayout* grid = new QGridLayout(this, 6, 4, 8, 8);
  QPushButton *btnOk, *btnCancel;
  QLabel *label;

  mMsgPart = NULL;

  resize(320, 190);
  if (aCaption) setCaption(aCaption);
  else setCaption(nls->translate("Message Part Properties"));

  mLblIcon = new QLabel(this);
  mLblIcon->resize(32, 32);
  grid->addMultiCellWidget(mLblIcon, 0, 1, 0, 0);

  mLblMimetype = new QLabel(this);
  grid->addWidget(mLblMimetype, 0, 1);

  mLblSize = new QLabel(this);
  grid->addWidget(mLblSize, 1, 1);

  label = new QLabel(nls->translate("Name:"), this);
  grid->addWidget(label, 2, 0);

  mEdtName = new QLineEdit(this);
  grid->addMultiCellWidget(mEdtName, 2, 2, 1, 3);

  label = new QLabel(nls->translate("Description:"), this);
  grid->addWidget(label, 3, 0);

  mEdtComment = new QLineEdit(this);
  grid->addMultiCellWidget(mEdtComment, 3, 3, 1, 3);

  label = new QLabel(nls->translate("Encoding:"), this);
  grid->addWidget(label, 4, 0);

  mCbxEncoding = new QComboBox(this);
  mCbxEncoding->insertItem(nls->translate("none (8bit)"));
  mCbxEncoding->insertItem(nls->translate("base 64"));
  mCbxEncoding->insertItem(nls->translate("quoted printable"));
  grid->addMultiCellWidget(mCbxEncoding, 4, 4, 1, 2);


  btnOk = new QPushButton(nls->translate("Ok"), this);
  connect(btnOk, SIGNAL(clicked()), SLOT(accept()));
  grid->addMultiCellWidget(btnOk, 5, 5, 0, 1);

  btnCancel = new QPushButton(nls->translate("Cancel"), this);
  connect(btnCancel, SIGNAL(clicked()), SLOT(reject()));
  grid->addMultiCellWidget(btnCancel, 5, 5, 2, 3);

  grid->activate();
}


//-----------------------------------------------------------------------------
KMMsgPartDlg::~KMMsgPartDlg()
{
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::setMsgPart(KMMessagePart* aMsgPart)
{
  unsigned int len, idx;
  QString lenStr, iconName, enc;

  mMsgPart = aMsgPart;
  assert(mMsgPart!=NULL);

  mEdtComment->setText(mMsgPart->contentDescription());
  mEdtName->setText(mMsgPart->name());
  mLblMimetype->setText(mMsgPart->typeStr()+"/"+mMsgPart->subtypeStr());

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
  QString str, body;
  int idx;

  if (!mMsgPart) return;

  kbp->busy();
  str = mEdtName->text();
  if (!str.isEmpty || !mMsgPart->name().isEmpty())
    mMsgPart->setName(str);

  str = mEdtComment->text();
  if (!str.isEmpty || !mMsgPart->contentDescription().isEmpty())
    mMsgPart->setContentDescription(str);

  idx = mCbxEncoding->currentItem();
  if (idx==1) str = "base64";
  else if (idx==2) str = "quoted printable";
  else str = "8bit";

  if (str != mMsgPart->cteStr())
  {
    body = mMsgPart->bodyDecoded();
    mMsgPart->setCteStr(str);
    mMsgPart->setEncodedBody(body);
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
