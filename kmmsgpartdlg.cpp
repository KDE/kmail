// kmmsgpartdlg.cpp

#include "kmmsgpartdlg.h"
#include "kmmsgpart.h"
#include "kmmsgbase.h"

#include "kmglobal.h"

#include "kbusyptr.h"
#include <kapp.h>
#include <kcombobox.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <unistd.h>
#include <assert.h>
#include <klocale.h>


//-----------------------------------------------------------------------------
KMMsgPartDlg::KMMsgPartDlg(const char* aCaption, bool readOnly):
  KMMsgPartDlgInherited(NULL, "msgpartdlg", TRUE), mIconPixmap()
{
  QGridLayout* grid = new QGridLayout(this, 6, 4, 8, 8);
  QPushButton *btnOk, *btnCancel;
  QLabel *label;
  int h, w1, w2;

  mMsgPart = NULL;

  if (aCaption) setCaption(aCaption);
  else setCaption(i18n("Message Part Properties"));

  mLblIcon = new QLabel(this);
  mLblIcon->resize(32, 32);
  grid->addMultiCellWidget(mLblIcon, 0, 1, 0, 0);

  //-----
  mEdtMimetype = new KComboBox(true, this);
  mEdtMimetype->setInsertionPolicy(QComboBox::NoInsertion);
  // here's only a small selection of what I think are very common mime types (dnaber, 2000-04-24):
  mEdtMimetype->insertItem("text/html");
  mEdtMimetype->insertItem("text/plain");
  mEdtMimetype->insertItem("image/gif");
  mEdtMimetype->insertItem("image/jpeg");
  mEdtMimetype->insertItem("image/png");
  mEdtMimetype->insertItem("application/octet-stream");
  mEdtMimetype->insertItem("application/x-gunzip");
  mEdtMimetype->insertItem("application/zip");
  h = mEdtMimetype->sizeHint().height();
  mEdtMimetype->setMinimumSize(100, h);
  mEdtMimetype->setMaximumSize(1024, h);
  grid->addMultiCellWidget(mEdtMimetype, 0, 0, 1, 3);
  connect(mEdtMimetype, SIGNAL(textChanged(const QString &)),
    SLOT(mimetypeChanged(const QString &)));

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
  grid->addMultiCellWidget(mCbxEncoding, 4, 4, 1, 3);

  if(readOnly)
    {mEdtMimetype->setEnabled(FALSE);
     mEdtName->setEnabled(FALSE);
     mEdtComment->setEnabled(FALSE);
     mCbxEncoding->setEnabled(FALSE);
    }
	

  //-----
  btnOk = new QPushButton(i18n("OK"), this);
  btnOk->adjustSize();
  btnOk->setMinimumSize(btnOk->sizeHint());
  connect(btnOk, SIGNAL(clicked()), SLOT(accept()));
  grid->addMultiCellWidget(btnOk, 5, 5, 1, 1);

  btnCancel = new QPushButton(i18n("Cancel"), this);
  btnCancel->adjustSize();
  btnCancel->setMinimumSize(btnCancel->sizeHint());
  connect(btnCancel, SIGNAL(clicked()), SLOT(reject()));
  grid->addMultiCellWidget(btnCancel, 5, 5, 2, 3);

  h  = btnOk->sizeHint().height();
  w1 = btnOk->sizeHint().width();
  w2 = btnCancel->sizeHint().width();
  if (w1 < w2) w1 = w2;
  if (w1 < 120) w1 = 120;
  btnOk->setMaximumSize(w1, h);
  btnOk->setFocus();
  btnCancel->setMaximumSize(w1, h);

  //-----
  grid->setColStretch(0, 0);
  grid->setColStretch(1, 10);
  grid->setColStretch(3, 10);
  grid->activate();

  resize(420, 190);
}


//-----------------------------------------------------------------------------
KMMsgPartDlg::~KMMsgPartDlg()
{
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::mimetypeChanged(const QString & name)
{
  if (name == "message/rfc822")
  {
    mCbxEncoding->setCurrentItem(0);
    mCbxEncoding->setEnabled(false);
  } else {
    mCbxEncoding->setEnabled(mEdtMimetype->isEnabled());
  }
}

//-----------------------------------------------------------------------------
void KMMsgPartDlg::setMsgPart(KMMessagePart* aMsgPart)
{
  unsigned int len, idx;
  QString lenStr, iconName, enc;

  mMsgPart = aMsgPart;
  assert(mMsgPart!=NULL);

  enc = mMsgPart->cteStr();
  if (enc=="base64") idx = 1;
  else if (enc=="quoted-printable") idx = 2;
  else idx = 0;
  mCbxEncoding->setCurrentItem(idx);

  mEdtComment->setText(mMsgPart->contentDescription());
  mEdtName->setText(mMsgPart->fileName());
  QString mimeType = mMsgPart->typeStr() + "/" + mMsgPart->subtypeStr();
  mEdtMimetype->setEditText(mimeType);
  mEdtMimetype->insertItem(mimeType, 0);

  len = mMsgPart->size();
  if (len > 9999) lenStr.sprintf("%u KB", (len>>10));
  else lenStr.sprintf("%u bytes", len);
  mLblSize->setText(lenStr);

  iconName = mMsgPart->iconName();
  mIconPixmap.load(iconName);
  mLblIcon->setPixmap(mIconPixmap);
  mLblIcon->resize(mIconPixmap.size());
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::applyChanges(void)
{
  QString str, type, subtype;
  QCString body;
  int idx;

  if (!mMsgPart) return;

  kernel->kbp()->busy();
  str = mEdtName->text();
  if (!str.isEmpty() || !mMsgPart->name().isEmpty())
  {
    mMsgPart->setName(str);
    QString encName = KMMsgBase::encodeRFC2231String(str, mMsgPart->charset());
    mMsgPart->setContentDisposition(QString("attachment; filename")
      + ((str != encName) ? "*" : "") +  "=\"" + encName + "\"");
  }

  str = mEdtComment->text();
  if (!str.isEmpty() || !mMsgPart->contentDescription().isEmpty())
    mMsgPart->setContentDescription(str);

  if (mEdtMimetype->currentText() == "message/rfc822")
  {
    str = "7bit";
  } else {
    idx = mCbxEncoding->currentItem();
    if (idx==1) str = "base64";
    else if (idx==2) str = "quoted-printable";
    else str = "8bit";
  }
  type = mEdtMimetype->currentText();
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
    body.duplicate( mMsgPart->bodyDecoded() );
    mMsgPart->setCteStr(str);
    mMsgPart->setBodyEncoded(body);
  }
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMMsgPartDlg::done(int rc)
{
  if (rc) applyChanges();
  KMMsgPartDlgInherited::done(rc);
}


//-----------------------------------------------------------------------------
#include "kmmsgpartdlg.moc"
