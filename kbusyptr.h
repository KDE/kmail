/* Animated Busy Pointer for KDE
 *
 * Author: Stefan Taferner <taferner@salzburg.co.at>
 * This code is under GPL
 */
#ifndef kbusyptr_h_
#define kbusyptr_h_

class KBusyPtr
{
public:
  KBusyPtr();
  virtual ~KBusyPtr();

  /** Show busy pointer. Subsequent calls increase the "busy level" */
  virtual void busy(void);

  /** Hide busy pointer if the "busy level" is reduced to zero. */
  virtual void idle(void);

  /** Returns true if the "busy level" is non zero. */
  virtual bool isBusy(void);

protected:
  int busyLevel;
};

#endif /*kbusyptr_h_*/
