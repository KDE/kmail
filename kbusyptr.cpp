// kbusyptr.cpp

#include "kbusyptr.h"
#include <kapp.h>
#include <qcursor.h>
#include <qtimer.h>
#include <assert.h>

//-----------------------------------------------------------------------------
KBusyPtr :: KBusyPtr ()
{
  app = KApplication::getKApplication();

  busyLevel  = 0;
  numCursors = 0;
  frameDelay = 500;
  cursorList = NULL;
  bitmapList = NULL;

  loadCursor("./pics/stopwatch.xbm","./pics/stopwatchMask.xbm");

  //connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
}


//-----------------------------------------------------------------------------
KBusyPtr :: ~KBusyPtr()
{
  if (cursorList) delete[] cursorList;
  if (bitmapList) delete[] bitmapList;
  cursorList = NULL;
  bitmapList = NULL;
}


//-----------------------------------------------------------------------------
void KBusyPtr :: busy (void)
{
  if (busyLevel <= 0)
  {
    currentCursor = 0;
    if (!cursorList)
    {
      app->setOverrideCursor(waitCursor);
    }
    else
    {
      app->setOverrideCursor(cursorList[currentCursor]);
      start(500);
    }
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
    app->restoreOverrideCursor();
  }
}


//-----------------------------------------------------------------------------
void KBusyPtr :: timerEvent (void)
{
  if (busyLevel <= 0) return;

  if (++currentCursor >= numCursors) currentCursor = 0;

  if (cursorList)
    app->setOverrideCursor(cursorList[currentCursor], TRUE);
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
    f = app->kdedir();
    f.detach();
    f += "/lib/pics/";
    f += filename;
  }
  rc = bm.load(f);
  if (!rc) printf ("ERROR: cannot load bitmap %s\n", f.data());
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

  if (bitmapList) delete[] bitmapList;
  bitmapList = new QBitmap[numCursors](16,16);
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
