// kbusyptr.cpp

#include "kbusyptr.h"

#include <qapplication.h>
#include <qcursor.h>

//-----------------------------------------------------------------------------
KBusyPtr :: KBusyPtr ()
    : busyLevel( 0 )
{
  busyLevel  = 0;
}


//-----------------------------------------------------------------------------
KBusyPtr :: ~KBusyPtr()
{
  while (busyLevel) idle();
}


//-----------------------------------------------------------------------------
void KBusyPtr :: busy (void)
{
  if (busyLevel <= 0)
  {
      QApplication::setOverrideCursor( QCursor(QCursor::WaitCursor) );
  }
  busyLevel++;
}


//-----------------------------------------------------------------------------
void KBusyPtr :: idle (void)
{
  busyLevel--;
  if (busyLevel < 0) busyLevel = 0;

  if (busyLevel <= 0)
  {
    QApplication::restoreOverrideCursor();
  }
}


//-----------------------------------------------------------------------------
bool KBusyPtr :: isBusy (void)
{
  return (busyLevel != 0);
}

