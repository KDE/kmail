/* kmail folder-list combo-box */
/* Author: Ronen Tzur <rtzur@shani.net> */

#include <qcombobox.h>
#include <qstringlist.h>
#include <qguardedptr.h>

#include "kmfoldercombobox.h"
#include "kmkernel.h"
#include "kmfoldermgr.h"

//-----------------------------------------------------------------------------

KMFolderComboBox::KMFolderComboBox( QWidget *parent, char *name )
  : QComboBox( parent, name )
{
  connect( this, SIGNAL( activated(int) ), this, SLOT( slotActivated(int) ) );
  refreshFolders();
}

KMFolderComboBox::KMFolderComboBox( bool rw, QWidget *parent, char *name )
  : QComboBox( rw, parent, name )
{
  connect( this, SIGNAL( activated(int) ), this, SLOT( slotActivated(int) ) );
  refreshFolders();
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::refreshFolders()
{
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  kernel->folderMgr()->createI18nFolderList( &names, &folders );
  
  KMFolder *folder = getFolder();
  this->clear();
  insertStringList( names );
  setFolder( folder );
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::setFolder( KMFolder *aFolder )
{
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  kernel->folderMgr()->createFolderList( &names, &folders );
  
  int idx = folders.findIndex( aFolder );
  setCurrentItem( idx >= 0 ? idx : 0 );
  
  mFolder = aFolder;
}

//-----------------------------------------------------------------------------

KMFolder *KMFolderComboBox::getFolder()
{
  if (mFolder)
    return mFolder;
  
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  kernel->folderMgr()->createI18nFolderList( &names, &folders );

  int idx = 0;
  QStringList::Iterator it;
  for ( it = names.begin(); it != names.end(); ++it ) {
    if ( ! (*it).compare( currentText() ) )
      return *folders.at( idx );
    idx++;
  }
  
  return kernel->draftsFolder();
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::slotActivated(int index)
{
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  kernel->folderMgr()->createFolderList( &names, &folders );
  
  mFolder = *folders.at( index );
  KMFolder *folder = mFolder;
  QString name = folder->name();
  printf("Slot Activated = %s\n", name.latin1());
}

//-----------------------------------------------------------------------------

#include "kmfoldercombobox.moc"
