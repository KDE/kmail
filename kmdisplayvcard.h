#ifndef KMDISPLAYVCARD_H
#define KMDISPLAYVCARD_H

#include <kapp.h>
#include <kdialog.h>

#include "vcard.h"

class KMDisplayVCard : public KDialog {
 public:
  KMDisplayVCard();
  virtual ~KMDisplayVCard();

  void setVCard(VCard *vc);

 private:
  VCard *_vc;

};


#endif
