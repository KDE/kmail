#ifndef KMPGPWRAP_H
#define KMPGPWRAP_H

#include <kpgp.h>

class KMpgpWrap : public Kpgp
{
public:
  KMpgpWrap();
  virtual ~KMpgpWrap();
  virtual void setBusy();
  virtual bool isBusy();
  virtual void idle();
};

#endif
