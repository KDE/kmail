#ifndef KMPGPWRAP_H
#define KMPGPWRAP_H

#include <kpgp.h>

/** This class is a wrapper around @ref Kpgp::Module (in libkdenetwork). It's
    only purpose is to teach @ref Kpgp::Module how to show the "busy"
    cursor.
    @short This is a wrapper around Kpgp::Module.
*/
class KMpgpWrap : public Kpgp::Module
{
public:
  KMpgpWrap();
  virtual ~KMpgpWrap();
  virtual void setBusy();
  virtual bool isBusy();
  virtual void idle();
};

#endif
