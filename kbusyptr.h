/* animated busy pointer handling
 */
#ifndef kbusyptr_h_
#define kbusyptr_h_

#include "kalarmtimer.h"

class KApplication;
class QCursor;

class KBusyPtr: public KAlarmTimer
{
  Q_OBJECT;

public:
  KBusyPtr(KApplication*);
  virtual ~KBusyPtr();

  virtual void busy(void);
  virtual void idle(void);

  virtual void loadCursor(const char* cursorName, const char* maskName);
	// Load cursor from given bitmap files. When the filename
	// is relative the $KDEDIR/lib/pics directory is searched

protected:
  virtual void timerEvent(void);

private:
  bool loadBitmap(QBitmap& bitmap, const QString& fileName);

protected:
  KApplication* app;
  int busyLevel;
  int numCursors;
  int frameDelay;
  int currentCursor;
  QCursor* cursorList;
  QBitmap* bitmapList;
};

#endif /*kbusyptr_h_*/
