#include "kmdisplayvcard.h"

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <klocale.h>

KMDisplayVCard::KMDisplayVCard(VCard *vc) {
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
QGridLayout *grid = new QGridLayout(page, 5, 2);
QLabel *name;
QLabel *value;
QString tmpstr;
QValueList<QString> values;

  //
  // Display the Name: field
  values = _vc->getValues(QString("n"));
  if (values.count() > 0) {
    tmpstr = "";
    for (unsigned int i = 1; i < values.count(); i++) {
      tmpstr += values[i];
      tmpstr += " ";
    }
    tmpstr += values[0];
    name = new QLabel(i18n("Name:"), page);
    value = new QLabel(tmpstr, page);
    grid->addWidget(name, 0, 0);
    grid->addWidget(value, 0, 1);
  } else {
    tmpstr = _vc->getValue(QString("fn"));
    name = new QLabel(i18n("Name:"), page);
    value = new QLabel(tmpstr, page);
    grid->addWidget(name, 0, 0);
    grid->addWidget(value, 0, 1);
  }

  //
  // Display the Organization: field
  tmpstr = _vc->getValue(QString("org"));
  name = new QLabel(i18n("Organization:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 1, 0);
  grid->addWidget(value, 1, 1);

  //
  // Display the Title: field
  tmpstr = _vc->getValue(QString("title"));
  name = new QLabel(i18n("Title:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 2, 0);
  grid->addWidget(value, 2, 1);

  //
  // Display the E-Mail: field
  tmpstr = _vc->getValue(QString("email"), QString("internet"));
  name = new QLabel(i18n("E-Mail:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 3, 0);
  grid->addWidget(value, 3, 1);

  //
  // Display the URL: field
  tmpstr = _vc->getValue(QString("url"));
  name = new QLabel(i18n("URL:"), page);
  value = new QLabel(tmpstr, page);
  grid->addWidget(name, 4, 0);
  grid->addWidget(value, 4, 1);

return page;
}


QWidget *KMDisplayVCard::getSecondTab() {
QFrame *page = new QFrame(this);
QGridLayout *grid = new QGridLayout(page, 5, 2);
QLabel *name;
QLabel *value;
QString tmpstr;
QValueList<QString> values;

return page;
}


QWidget *KMDisplayVCard::getThirdTab() {
QFrame *page = new QFrame(this);
QGridLayout *grid = new QGridLayout(page, 5, 2);
QLabel *name;
QLabel *value;
QString tmpstr;
QValueList<QString> values;

return page;
}



