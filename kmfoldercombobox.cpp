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
  mImapShown = true;
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

void KMFolderComboBox::showImapFolders(bool shown)
{
  mImapShown = shown;
  refreshFolders();
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::createFolderList(QStringList *names,
                                        QValueList<QGuardedPtr<KMFolder> > *folders)
{
  if (mImapShown)
    kernel->imapFolderMgr()->createI18nFolderList( names, folders );

  kernel->folderMgr()->createFolderList( names, folders );
  uint i = 0;
  while (i < folders->count())
  {
    if ((*(folders->at(i)))->isSystemFolder()
      && (*(folders->at(i)))->protocol() != "imap")
    {
      folders->remove(folders->at(i));
      names->remove(names->at(i));
    }
    else i++;
  }

  folders->prepend(kernel->draftsFolder());
  folders->prepend(kernel->trashFolder());
  folders->prepend(kernel->sentFolder());
  if (mOutboxShown) folders->prepend(kernel->outboxFolder());
  folders->prepend(kernel->inboxFolder());
  for (int i = ((mOutboxShown) ? 4 : 3); i >= 0; i--)
    names->prepend((*(folders->at(i)))->label());
}

//-----------------------------------------------------------------------------

void KMFolderComboBox::refreshFolders()
{
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  createFolderList( &names, &folders );
  
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
  createFolderList( &names, &folders );
  
  int idx = folders.findIndex( aFolder );
  if (idx == -1)
    idx = folders.findIndex( kernel->draftsFolder() );
  setCurrentItem( idx >= 0 ? idx : 0 );
  
  mFolder = aFolder;
}

void KMFolderComboBox::setFolder( const QString &idString )
{
  KMFolder *folder = kernel->folderMgr()->findIdString( idString );
  if (!folder) folder = kernel->imapFolderMgr()->findIdString( idString );
  setFolder( folder );
}

//-----------------------------------------------------------------------------

KMFolder *KMFolderComboBox::getFolder()
{
  if (mFolder)
    return mFolder;
  
  QStringList names;
  QValueList<QGuardedPtr<KMFolder> > folders;
  createFolderList( &names, &folders );

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
  createFolderList( &names, &folders );
  
  mFolder = *folders.at( index );
  KMFolder *folder = mFolder;
  QString name = folder->name();
}

//-----------------------------------------------------------------------------

#include "kmfoldercombobox.moc"
