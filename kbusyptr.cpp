// kbusyptr.cpp

#include "kbusyptr.h"
#include <kapp.h>
#include <kcursor.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <qtimer.h>
#include <assert.h>
#include <kstddirs.h>
#include <kdebug.h>

//-----------------------------------------------------------------------------
KBusyPtr :: KBusyPtr ()
{
  app = KApplication::kApplication();

  busyLevel  = 0;
  numCursors = 0;
  frameDelay = 500;
  cursorList = NULL;
  //animated   = TRUE;
  animated   = FALSE;

  loadCursor("stopwatch.xbm","stopwatchMask.xbm");

  //connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
}


//-----------------------------------------------------------------------------
KBusyPtr :: ~KBusyPtr()
{
  while (busyLevel) idle();
  if (cursorList) delete[] cursorList;
  cursorList = NULL;
}


//-----------------------------------------------------------------------------
void KBusyPtr :: busy (void)
{
  if (busyLevel <= 0)
  {
    currentCursor = 0;
    if (!cursorList)
    {
      if (animated) kapp->setOverrideCursor(waitCursor);
      else kapp->setOverrideCursor(KCursor::waitCursor());
    }
    else
    {
      kapp->setOverrideCursor(cursorList[currentCursor]);
      if (animated) start(frameDelay);
    }
    //    kapp->processEvents(200);
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
    stop();
    kapp->restoreOverrideCursor();
    //    kapp->processEvents(200);
  }
}


//-----------------------------------------------------------------------------
bool KBusyPtr :: isBusy (void)
{
  return (busyLevel != 0);
}


//-----------------------------------------------------------------------------
void KBusyPtr :: timerEvent (void)
{
  if (busyLevel <= 0) return;

  if (++currentCursor >= numCursors) currentCursor = 0;

  if (cursorList)
    kapp->setOverrideCursor(cursorList[currentCursor], TRUE);
}


//-----------------------------------------------------------------------------
void KBusyPtr :: stopAnimation (void)
{
  if (animated) stop();
  animated = FALSE;
}


//-----------------------------------------------------------------------------
void KBusyPtr :: continueAnimation (void)
{
  animated = TRUE;
  start(frameDelay);
}


//-----------------------------------------------------------------------------
bool KBusyPtr :: loadBitmap (QBitmap& bm, const QString& filename)
{
  QString f;
  bool rc;

  if (filename[0]=='/' || filename[0]=='.')
  {
    f = filename;
  }
  else 
  {
    f = locate("data", "kmail/pics/" + filename);
  }
  rc = bm.load(f);
  if (!rc) kdDebug(5006) << "ERROR: cannot load bitmap %s\n" << f << endl;
  return rc;
}


//-----------------------------------------------------------------------------
void KBusyPtr :: loadCursor (const char* cursorName,const char* maskName)
{
  int i, ix, iy, numX, numY, x, y;
  QBitmap map, mask;
  QBitmap cmap(16,16), cmask(16,16);

  numCursors = 0;

  if (!loadBitmap(map,cursorName)) return;
  if (!loadBitmap(mask,maskName)) return;

  numX = map.width() >> 4;
  numY = map.height() >> 4;
  numCursors = numX * numY;

  if (cursorList) delete[] cursorList;
  cursorList = new QCursor[numCursors];

  for (i=0,iy=0,y=0; iy<numY; iy++, y+=16)
  {
    for (ix=0,x=0; ix<numX; ix++, x+=16, i++)
    {
      bitBlt(&cmap, 0, 0, &map, x, y, 16, 16);
      bitBlt(&cmask, 0, 0, &mask, x, y, 16, 16);
      cursorList[i] = QCursor(cmap, cmask, 8, 8);
    }
  }
}

#include "kbusyptr.moc"
