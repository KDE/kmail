/* Mail Filter Manager
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfiltermgr_h
#define kmfiltermgr_h

#include "kmfolder.h"
#include "kmfilter.h"

#include <qptrlist.h>
#include <qguardedptr.h>

class KMFilterDlg;

#define KMFilterMgrInherited QPtrList<KMFilter>
class KMFilterMgr: public QPtrList<KMFilter>
{
public:
  KMFilterMgr(bool popFilter = false);
  virtual ~KMFilterMgr();

  enum FilterSet { NoSet = 0x0, Inbound = 0x1, Outbound = 0x2, All = 0x80000000 };

  /** Reload filter rules from config file. */
  virtual void readConfig(void);

  /** Store filter rules in config file. */
  virtual void writeConfig(bool withSync=TRUE);

  /** Open an edit dialog. */
  virtual void openDialog( QWidget *parent );

  /** Open an edit dialog, create a new filter and preset the first
      rule with "field equals value" */
  virtual void createFilter( const QCString field, const QString value );

  /** Process given message by applying the filter rules one by
      one. You can select which set of filters (incoming or outgoing)
      should be used.

      @param msg The message to process.
      @param aSet Select the filter set to use.
      @return 2 if a critical error occurred (eg out of disk space)
      1 if the caller is still owner of the message and
      0 otherwise. If the caller does not any longer own the message
      he *must* not delete the message or do similar stupid things. ;-)
  */
  virtual int process(KMMessage* msg, FilterSet aSet=Inbound);

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

  /** Called at the beginning of an filter list update. Currently a
      no-op */
  void beginUpdate() {}

  /** Called at the end of an filter list update. Currently a no-op */
  void endUpdate() {}

  /** Output all rules to stdout */
  virtual void dump(void);

  /** Called from the folder manager when a folder is removed.
    Tests if the folder aFolder is used in any action. Changes
    to aNewFolder folder in this case. Returns TRUE if a change
    occured. */
  virtual bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);

  /** Called from the folder manager when a new folder has been
      created. Forwards this to the filter dialog if that is open. */
  virtual void folderCreated(KMFolder*) {}

  /** Set the global option 'Show Download Later Messages' */
  virtual void setShowLaterMsgs(bool);

  /** Get the global option 'Show Download Later Messages' */
  virtual bool showLaterMsgs();

private:
  QGuardedPtr<KMFilterDlg> mEditDialog;
  QPtrList<KMFolder> mOpenFolders;
  bool bPopFilter;
  bool mShowLater;
};

#endif /*kmfiltermgr_h*/
