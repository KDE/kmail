#include "kmdisplayvcard.moc"

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <klocale.h>

KMDisplayVCard::KMDisplayVCard(VCard *vc, QWidget *parent, const char *name) : KTabCtl(parent, name) {
  _vc = vc;
  BuildInterface();
}


KMDisplayVCard::~KMDisplayVCard() {
  delete _vc;
}
 

void KMDisplayVCard::setVCard(VCard *vc) {
  if (_vc) delete _vc;
  _vc = vc;
}
  

void KMDisplayVCard::BuildInterface() {
  addTab(getFirstTab(), i18n("Name"));
  addTab(getSecondTab(), i18n("Address"));
  addTab(getThirdTab(), i18n("Misc"));
}


QWidget *KMDisplayVCard::getFirstTab() {
QFrame *page = new QFrame(this);
QGridLayout *grid = new QGridLayout(page, 7, 2);
QLabel *name;
QLabel *value;
QString tmpstr;
QValueList<QString> values;

  //
  // Display the Name: field
  tmpstr = _vc->getValue(VCARD_FN);
  if (tmpstr.length() == 0) {
    values = _vc->getValues(VCARD_N);
    tmpstr = "";
    if (values.count() >= 4) {
      tmpstr += values[3];
      tmpstr += " ";
    }
    if (values.count() >= 2) {
      tmpstr += values[1];
      tmpstr += " ";
    }
    if (values.count() >= 3) {
      tmpstr += values[2];
      tmpstr += " ";
    }
    tmpstr += values[0];
    if (values.count() >= 5) {
      tmpstr += " ";
      tmpstr += values[4];
    }
  }
  QString nn = _vc->getValue(VCARD_NICKNAME);
  if (nn != "") {
    tmpstr += " (";
    tmpstr += nn;
    tmpstr += ")";
  }
  name = new QLabel(i18n("Name:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 0, 0);
  grid->addWidget(value, 0, 1);

  //
  // Display the Organization: field
  tmpstr = _vc->getValue(VCARD_ORG);
  name = new QLabel(i18n("Organization:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 1, 0);
  grid->addWidget(value, 1, 1);

  //
  // Display the Title: field
  tmpstr = _vc->getValue(VCARD_TITLE);
  name = new QLabel(i18n("Title:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 2, 0);
  grid->addWidget(value, 2, 1);

  //
  // Display the E-Mail: field
  tmpstr = _vc->getValue(VCARD_EMAIL, VCARD_EMAIL_INTERNET);
  name = new QLabel(i18n("E-Mail:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 3, 0);
  grid->addWidget(value, 3, 1);

  //
  // Display the URL: field
  tmpstr = _vc->getValue(VCARD_URL);
  name = new QLabel(i18n("URL:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 4, 0);
  grid->addWidget(value, 4, 1);

  //
  // Display the Birthday: field
  tmpstr = _vc->getValue(VCARD_BDAY);
  name = new QLabel(i18n("Birthday:"), page);
  // FIXME: this needs to be formatted
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 5, 0);
  grid->addWidget(value, 5, 1);

return page;
}



#define DO_PHONENUMBER(X,Y)  thisnum = _vc->getValue(VCARD_TEL, X);  \
                             if (thisnum.length() > 0) {             \
                               name = new QLabel(i18n(Y), page);     \
                               grid->addWidget(name, row, 0);        \
                               value = new QLabel(thisnum, page);    \
                               grid->addWidget(value, row++, 1);     \
                             }

QWidget *KMDisplayVCard::getSecondTab() {
QFrame *page = new QFrame(this);
QGridLayout *grid = new QGridLayout(page, 5, 2);
QLabel *name;
QLabel *value;
QString tmpstr;
QValueList<QString> values;

  //
  // Add the Address[es]
  name = new QLabel(i18n("Address:"), page);
  grid->addWidget(name, 0, 0);
  mAddrList = new QListBox(page);
  mAddrList->setSelectionMode(QListBox::Single);
  grid->addWidget(mAddrList, 0, 1);
  QFrame *addrPage = new QFrame(page);
  QGridLayout *addrGrid = new QGridLayout(addrPage, 5, 4);
  name = new QLabel(i18n("PO BOX:"), addrPage); 
  addrGrid->addWidget(name, 0, 0);
  mAddr_PO = new QLabel("", addrPage); 
  addrGrid->addMultiCellWidget(mAddr_PO, 0, 0, 1, 3);
  name = new QLabel(i18n("Address 1:"), addrPage); 
  addrGrid->addWidget(name, 1, 0);
  mAddr_addr1 = new QLabel("", addrPage); 
  addrGrid->addMultiCellWidget(mAddr_addr1, 1, 1, 1, 3);
  name = new QLabel(i18n("Address 2:"), addrPage); 
  addrGrid->addWidget(name, 2, 0);
  mAddr_addr2 = new QLabel("", addrPage); 
  addrGrid->addMultiCellWidget(mAddr_addr2, 2, 2, 1, 3);
  name = new QLabel(i18n("City:"), addrPage); 
  addrGrid->addWidget(name, 3, 0);
  mAddr_city = new QLabel("", addrPage); 
  addrGrid->addWidget(mAddr_city, 3, 1);
  name = new QLabel(i18n("Province:"), addrPage); 
  addrGrid->addWidget(name, 3, 2);
  mAddr_prov = new QLabel("", addrPage); 
  addrGrid->addWidget(mAddr_prov, 3, 3);
  name = new QLabel(i18n("Country:"), addrPage); 
  addrGrid->addWidget(name, 4, 0);
  mAddr_country = new QLabel("", addrPage); 
  addrGrid->addWidget(mAddr_country, 4, 1);
  name = new QLabel(i18n("Postal Code:"), addrPage); 
  addrGrid->addWidget(name, 4, 2);
  mAddr_postal = new QLabel("", addrPage); 
  addrGrid->addWidget(mAddr_postal, 4, 3);
  
  // set the types that are available
  if (!_vc->getValues(VCARD_ADR).isEmpty()) {
    mAddrList->insertItem(i18n("Default")); 
    mAddresses.append(_vc->getValues(VCARD_ADR));
  }
  if (!_vc->getValues(VCARD_ADR, VCARD_ADR_DOM).isEmpty()) {
    mAddrList->insertItem(i18n("Domestic")); 
    mAddresses.append(_vc->getValues(VCARD_ADR, VCARD_ADR_DOM));
  }
  if (!_vc->getValues(VCARD_ADR, VCARD_ADR_INTL).isEmpty()) {
    mAddrList->insertItem(i18n("International")); 
    mAddresses.append(_vc->getValues(VCARD_ADR, VCARD_ADR_INTL));
  }
  if (!_vc->getValues(VCARD_ADR, VCARD_ADR_POSTAL).isEmpty()) {
    mAddrList->insertItem(i18n("Postal")); 
    mAddresses.append(_vc->getValues(VCARD_ADR, VCARD_ADR_POSTAL));
  }
  if (!_vc->getValues(VCARD_ADR, VCARD_ADR_HOME).isEmpty()) {
    mAddrList->insertItem(i18n("Home")); 
    mAddresses.append(_vc->getValues(VCARD_ADR, VCARD_ADR_HOME));
  }
  if (!_vc->getValues(VCARD_ADR, VCARD_ADR_WORK).isEmpty()) {
    mAddrList->insertItem(i18n("Work")); 
    mAddresses.append(_vc->getValues(VCARD_ADR, VCARD_ADR_WORK));
  }
  if (!_vc->getValues(VCARD_ADR, VCARD_ADR_PREF).isEmpty()) {
    mAddrList->insertItem(i18n("Preferred")); 
    mAddresses.append(_vc->getValues(VCARD_ADR, VCARD_ADR_PREF));
  }

  mAddrList->setSelected(0,true);
  mAddrList->setMinimumSize(mAddrList->minimumSizeHint());
  connect(mAddrList, SIGNAL(selectionChanged()), SLOT(slotChangeAddress()));
  slotChangeAddress();   // set the first one

  grid->addWidget(addrPage, 1, 1);
  name = new QLabel(i18n("Phone Numbers"), page);
  grid->addWidget(name, 2, 0);
  // try to add each type of phone number if we have them
  QString thisnum;
  int row = 3;
  DO_PHONENUMBER(VCARD_TEL_HOME, "Home");
  DO_PHONENUMBER(VCARD_TEL_WORK, "Work");
  DO_PHONENUMBER(VCARD_TEL_PREF, "Preferred");
  DO_PHONENUMBER(VCARD_TEL_VOICE, "Voice");
  DO_PHONENUMBER(VCARD_TEL_FAX, "Fax");
  DO_PHONENUMBER(VCARD_TEL_MSG, "Message");
  DO_PHONENUMBER(VCARD_TEL_CELL, "Cellular");
  DO_PHONENUMBER(VCARD_TEL_PAGER, "Pager");
  DO_PHONENUMBER(VCARD_TEL_BBS, "BBS");
  DO_PHONENUMBER(VCARD_TEL_MODEM, "Modem");
  DO_PHONENUMBER(VCARD_TEL_CAR, "Car");
  DO_PHONENUMBER(VCARD_TEL_ISDN, "ISDN");
  DO_PHONENUMBER(VCARD_TEL_VIDEO, "Video");
  DO_PHONENUMBER(VCARD_TEL_PCS, "PCS");
  
return page;
}

#undef DO_PHONENUMBER

QWidget *KMDisplayVCard::getThirdTab() {
QFrame *page = new QFrame(this);
QGridLayout *grid = new QGridLayout(page, 5, 2);
QLabel *name;
QLabel *value;
QString tmpstr;
QValueList<QString> values;

return page;
}


void KMDisplayVCard::slotChangeAddress() {
int c = mAddrList->currentItem();

  if (mAddresses.count() > c) {
    mAddr_PO->setText(mAddresses[c][0]);
    mAddr_PO->resize(mAddr_PO->sizeHint());

    mAddr_addr1->setText(mAddresses[c][2]);
    mAddr_addr1->resize(mAddr_addr1->sizeHint());

    mAddr_addr2->setText(mAddresses[c][1]);
    mAddr_addr2->resize(mAddr_addr2->sizeHint());

    mAddr_city->setText(mAddresses[c][3]);
    mAddr_city->resize(mAddr_city->sizeHint());

    mAddr_prov->setText(mAddresses[c][4]);
    mAddr_prov->resize(mAddr_prov->sizeHint());

    mAddr_country->setText(mAddresses[c][6]);
    mAddr_country->resize(mAddr_country->sizeHint());

    mAddr_postal->setText(mAddresses[c][5]);
    mAddr_postal->resize(mAddr_postal->sizeHint());
  }
}

