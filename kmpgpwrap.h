#ifndef KMPGPWRAP_H
#define KMPGPWRAP_H

#include <kpgp.h>

/** This class is a wrapper around @ref Kpgp (in libkdenetwork). It's
    only purpose is to teach @ref Kpgp how to show the "busy"
    cursor.
    @short This wrapper around Kpgp.
*/
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
