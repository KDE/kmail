#include "kmkernel.h"
#include "kmpgpwrap.h"
#include "kbusyptr.h"

KMpgpWrap::KMpgpWrap()
  : Kpgp::Module()
{
}

KMpgpWrap::~KMpgpWrap()
{
}

void KMpgpWrap::setBusy()
{
  kernel->kbp()->busy();
}

bool KMpgpWrap::isBusy()
{
  return kernel->kbp()->isBusy();
}

void KMpgpWrap::idle()
{
  kernel->kbp()->idle();
}

