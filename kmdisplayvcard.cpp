#include "kmdisplayvcard.h"

KMDisplayVCard::KMDisplayVCard() {
  _vc = NULL;
}


KMDisplayVCard::~KMDisplayVCard() {
  delete _vc;
}
 

void KMDisplayVCard::setVCard(VCard *vc) {
  if (_vc) delete _vc;
  _vc = vc;
}
  
