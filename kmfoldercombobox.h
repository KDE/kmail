/* kmail folder-list combo-box
 * A specialized QComboBox widget that refreshes its contents when
 * the folder list changes.
 */

#ifndef __KMFOLDERCOMBOBOX
#define __KMFOLDERCOMBOBOX

#include "kmfolder.h"

#include <qcombobox.h>
#include <qguardedptr.h>

class KMFolderComboBox : public QComboBox
{
  Q_OBJECT

public:
  KMFolderComboBox( QWidget *parent = 0, char *name = 0 );
  KMFolderComboBox( bool rw, QWidget *parent = 0, char *name = 0 );

  /** Select whether the outbox folder is shown.  Default is yes. */
  void showOutboxFolder(bool shown);

  /** Select whether the IMAP folders should be shown.  Default is yes. */
  void showImapFolders(bool shown);

  void setFolder( KMFolder *aFolder );
  void setFolder( const QString &idString );
  KMFolder *getFolder();

public slots:
  /** Refresh list of folders in the combobox. */
  void refreshFolders();

private slots:
  void slotActivated(int index);

private:
  /** Create folder list using the folder manager. */
  void createFolderList(QStringList *names,
                        QValueList<QGuardedPtr<KMFolder> > *folders);
  void init();

  QGuardedPtr<KMFolder> mFolder;
  bool mOutboxShown;
  bool mImapShown;
  int mSpecialIdx;
};

#endif /* __KMFOLDERCOMBOBOX */
