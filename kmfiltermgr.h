/* Mail Filter Manager
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfiltermgr_h
#define kmfiltermgr_h

#include <qlist.h>
#include "kmfolder.h"
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
  virtual void openDialog( QWidget *parent );
  virtual void createFilter( const QString field, const QString value );

  /** Process given message by applying the filter rules one by one.
    Returns 2 if a critical error occurred (eg out of disk space) 
    1 if the caller is still owner of the message and
    0 otherwise. If the caller does not any longer own the message
    he *must* not delete the message or do similar stupid things. ;-) */
  virtual int process(KMMessage* msg);

  /** Call this method after processing messages with process(). 
    Shall be called after all messages are processed. This method
    closes all folders that have been temporarily opened with
    tempOpenFolder(). */
  virtual void cleanup(void);

  /** Open given folder and mark it as temporarily open. The folder
    will be closed upon next call of cleanip(). This method is
    usually only called from within filter actions during process().
    Returns returncode from KMFolder::open() call. */
  virtual int tempOpenFolder(KMFolder* aFolder);

  /** Output all rules to stdout */
  virtual void dump(void);

  /** Called from the folder manager when a folder is removed.
    Tests if the folder aFolder is used in any action. Changes 
    to aNewFolder folder in this case. Returns TRUE if a change
    occured. */
  virtual bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);

protected:
  friend class KMFilterMgrDlg;


public slots:
    /** Connected to the dialog to detect when it ha been destroyed */
    void dialogDestroyed();

private:
  KMFilterDlg* mEditDialog;
  QList<KMFolder> mOpenFolders;
};

#endif /*kmfiltermgr_h*/
