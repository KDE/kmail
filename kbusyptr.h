/* Animated Busy Pointer for KDE
 *
 * Author: Stefan Taferner <taferner@salzburg.co.at>
 * This code is under GPL
 */
#ifndef kbusyptr_h_
#define kbusyptr_h_

#include "kalarmtimer.h"

class KApplication;
class QCursor;

class KBusyPtr: public KAlarmTimer
{
  Q_OBJECT

public:
  KBusyPtr();
  virtual ~KBusyPtr();

  /** Show busy pointer. Subsequent calls increase the "busy level" */
  virtual void busy(void);

  /** Hide busy pointer if the "busy level" is reduced to zero. */
  virtual void idle(void);

  /** Stop pointer animation. This is necessary for some system calls. */
  virtual void stopAnimation(void);

  /** Continue pointer animation. */
  virtual void continueAnimation(void);

  /** Load cursor from given bitmap files. When the filename is relative
    the $KDEDIR/lib/pics directory is searched. */
  virtual void loadCursor(const char* cursorName, const char* maskName);



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
  bool animated;
  QCursor* cursorList;
};

#endif /*kbusyptr_h_*/
