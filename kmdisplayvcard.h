#ifndef KMDISPLAYVCARD_H
#define KMDISPLAYVCARD_H

#include <kapp.h>
#include <kdialog.h>
#include <ktabctl.h>
#include <qwidget.h>
#include <qlistbox.h>
#include <qlabel.h>

#include "vcard.h"

class KMDisplayVCard : public KTabCtl {
Q_OBJECT

 public:
  KMDisplayVCard(VCard *vc, QWidget *parent = 0, const char *name = 0);
  virtual ~KMDisplayVCard();

  void setVCard(VCard *vc);

 public slots:
  void slotChangeAddress();

 private:
  VCard *_vc;
  QListBox *mAddrList;
  QLabel *mAddr_PO, *mAddr_addr1, *mAddr_addr2, *mAddr_city, *mAddr_prov,
         *mAddr_country, *mAddr_postal;
  QValueList< QValueList<QString> > mAddresses;

  void BuildInterface();
  QWidget *getFirstTab();
  QWidget *getSecondTab();
  QWidget *getThirdTab();


};


#endif
