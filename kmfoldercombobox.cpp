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
  mOutboxShown = true;
  refreshFolders();
  connect( this, SIGNAL( activated(int) ), this, SLOT( slotActivated(int) ) );  
  connect( kernel->folderMgr(), SIGNAL(changed()), this, SLOT(refreshFolders()) );
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::showOutboxFolder(bool shown)
{
  mOutboxShown = shown;
  refreshFolders();
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::createFolderList(QStringList *names,
                                        QValueList<QGuardedPtr<KMFolder> > *folders,
                                        bool i18nized)
{
  if (i18nized)
    kernel->folderMgr()->createI18nFolderList( names, folders );
  else
    kernel->folderMgr()->createFolderList( names, folders );
  
  if (!mOutboxShown) {
    int idx = folders->findIndex( kernel->outboxFolder() );
    if (idx != -1) {
      names->remove( names->at( idx ) );
      folders->remove( folders->at( idx ) );
    }
  }
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::refreshFolders()
{
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  createFolderList( &names, &folders, true );
  
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
  createFolderList( &names, &folders, false );
  
  int idx = folders.findIndex( aFolder );
  if (idx == -1)
    idx = folders.findIndex( kernel->draftsFolder() );
  setCurrentItem( idx >= 0 ? idx : 0 );
  
  mFolder = aFolder;
}

void KMFolderComboBox::setFolder( QString &idString )
{
  KMFolder *folder = kernel->folderMgr()->findIdString( idString );
  setFolder( folder );
}

//-----------------------------------------------------------------------------

KMFolder *KMFolderComboBox::getFolder()
{
  if (mFolder)
    return mFolder;
  
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  createFolderList( &names, &folders, true );

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
  createFolderList( &names, &folders, false );
  
  mFolder = *folders.at( index );
  KMFolder *folder = mFolder;
  QString name = folder->name();
}

//-----------------------------------------------------------------------------

#include "kmfoldercombobox.moc"
