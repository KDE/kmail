#ifndef KMDISPLAYVCARD_H
#define KMDISPLAYVCARD_H

#include <kapp.h>
#include <kdialog.h>
#include <ktabctl.h>
#include <qwidget.h>

#include "vcard.h"

class KMDisplayVCard : public KTabCtl {
 public:
  KMDisplayVCard(VCard *vc);
  virtual ~KMDisplayVCard();

  void setVCard(VCard *vc);

 private:
  VCard *_vc;

  void BuildInterface();
  QWidget *getFirstTab();
  QWidget *getSecondTab();
  QWidget *getThirdTab();

};


#endif
