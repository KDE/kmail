/* Mail Filter Manager
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfiltermgr_h
#define kmfiltermgr_h

#include <qlist.h>
#include "kmfilter.h"

class KMFilterDlg;

#define KMFilterMgrInherited QList<KMFilter>
class KMFilterMgr: public QList<KMFilter>
{
public:
  KMFilterMgr();
  virtual ~KMFilterMgr();

  /** Reload filter rules from config file. */
  virtual void readConfig(void);

  /** Store filter rules in config file. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Open an edit dialog. */
  virtual void openDialog(void);

  /** Process given message by applying the filter rules one by one.
   * Returns TRUE if the caller is still owner of the message and
   * FALSE otherwise. If the caller does not any longer own the message
   * he *must* not delete the message or do similar stupid things. ;-)
   */
  virtual bool process(KMMessage* msg);

protected:
  friend class KMFilterMgrDlg;

  // Called from the dialog to signal that it is gone.
  void dialogClosed(void);

private:
  KMFilterDlg* mEditDialog;
};

#endif /*kmfiltermgr_h*/
