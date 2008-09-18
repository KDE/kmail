/* kmail folder-list combo-box
 * A specialized KComboBox widget that refreshes its contents when
 * the folder list changes.
 */

#ifndef __KMFOLDERCOMBOBOX
#define __KMFOLDERCOMBOBOX

#include "kmfolder.h"

#include <QPointer>
#include <QList>

#include <kcombobox.h>

class KMFolderComboBox : public KComboBox
{
  Q_OBJECT

public:
  KMFolderComboBox( QWidget *parent = 0 );

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
                        QList<QPointer<KMFolder> > *folders);
  void init();

  QPointer<KMFolder> mFolder;
  bool mOutboxShown;
  bool mImapShown;
  int mSpecialIdx;
};

#endif /* __KMFOLDERCOMBOBOX */
