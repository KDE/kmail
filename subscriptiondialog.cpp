/*  -*- c++ -*-
    subscriptiondialog.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "subscriptiondialog.h"
#include "kmmessage.h"

#include <klocale.h>
#include <kdebug.h>


namespace KMail {

SubscriptionDialog::SubscriptionDialog( QWidget *parent, const QString &caption,
    KAccount * acct )
  : KSubscription( parent, caption, acct, User1, QString::null, true )
{
  // hide unneeded checkboxes
  hideTreeCheckbox();
  hideNewOnlyCheckbox();

  // hide the description column
  groupView->setColumnWidthMode(mDescrColumn, QListView::Manual);
  groupView->hideColumn(mDescrColumn);

  // connect to folderlisting
  ImapAccountBase* iaccount = static_cast<ImapAccountBase*>(acct);
  connect(iaccount, SIGNAL(receivedFolders(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)),
      this, SLOT(slotListDirectory(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &)));

  // ok-button
  connect(this, SIGNAL(okClicked()), SLOT(slotSave()));

  // reload-list button
  connect(this, SIGNAL(user1Clicked()), SLOT(slotLoadFolders()));

  // get the folders
  slotLoadFolders();
}

//------------------------------------------------------------------------------
void SubscriptionDialog::slotListDirectory( QStringList mSubfolderNames,
                                            QStringList mSubfolderPaths,
                                            QStringList mSubfolderMimeTypes,
                                            const ImapAccountBase::jobData & jobData )
{
  bool onlySubscribed = jobData.onlySubscribed;
  ImapAccountBase::ListType type = onlySubscribed ? 
    ImapAccountBase::ListSubscribedNoCheck : ImapAccountBase::List;
  ImapAccountBase* ai = static_cast<ImapAccountBase*>(mAcct);
  GroupItem *tmp = 0;
  for (uint i = 0; i < mSubfolderNames.count(); ++i)
  {
    GroupItem *item = 0;
    if (!onlySubscribed && mSubfolderPaths.size() > 0)
    {
      // the account does not know the delimiter
      if (mDelimiter.isEmpty())
      {
        int start = mSubfolderPaths[i].findRev(mSubfolderNames[i]);
        if (start > 1)
          mDelimiter = mSubfolderPaths[i].mid(start-1, 1);
      }

      // get the parent
      GroupItem *parent = 0;
      GroupItem *oldItem = 0;
      QString parentPath;
      findParentItem( mSubfolderNames[i], mSubfolderPaths[i], parentPath, &parent, &oldItem );

      if (!parent && parentPath != "/")
      {
        // the parent is not available and it's no root-item
        // this happens when the folders do not arrive in hierarchical order
        // so we create each parent in advance
        QStringList folders = QStringList::split(mDelimiter, parentPath);
        uint i = 0;
        for ( QStringList::Iterator it = folders.begin(); it != folders.end(); ++it ) 
        {
          QString name = *it;
          if (name.startsWith("/"))
            name = name.right(name.length()-1);
          if (name.endsWith("/"))
            name.truncate(name.length()-1);
          KGroupInfo info(name);
          if (("/"+name+"/") == ai->prefix()) 
          {
            ++i;
            continue;
          }
          info.subscribed = false;

          QStringList tmpPath;
          for ( uint j = 0; j <= i; ++j )
            tmpPath << folders[j];
          QString path = tmpPath.join(mDelimiter);
          if (!path.startsWith("/"))
            path = "/" + path;
          if (!path.endsWith("/"))
            path = path + "/";
          info.path = path;
          info.description = path;
          item = 0;
          if (folders.count() > 1)
          {
            // we have to create more then one level, so better check if this
            // folder already exists somewhere
            QListViewItemIterator it( groupView );
            if (parent) 
              it = (QListViewItemIterator( parent ));
            while ( it.current() != 0 )
            {
              tmp = static_cast<GroupItem*>(it.current());
              if (tmp->info().path == path) {
                item = tmp;
                break;
              }
              ++it;
            }
          }
          // as these items are "dummies" we create them non-checkable
          if (!item)
          {
            if (parent)
              item = new GroupItem(parent, info, this, false);
            else
              item = new GroupItem(folderTree(), info, this, false);
          }

          parent = item;
          ++i;
        } // folders
      } // parent
    
      KGroupInfo info(mSubfolderNames[i]);
      if (mSubfolderNames[i].upper() == "INBOX" &&
          mSubfolderPaths[i] == "/INBOX/")
        info.name = i18n("inbox");
      info.subscribed = false;
      info.path = mSubfolderPaths[i];
      info.description = mSubfolderPaths[i];
      // only checkable when the folder is selectable
      bool checkable = ( mSubfolderMimeTypes[i] == "inode/directory" ) ? false : true;
      // create a new item
      if (parent)
        item = new GroupItem(parent, info, this, checkable);
      else
        item = new GroupItem(folderTree(), info, this, checkable);

      if (oldItem)
      {
        // move the old childs to the new item
        QPtrList<QListViewItem> itemsToMove;
        QListViewItem * myChild = oldItem->firstChild();
        while (myChild)
        {
          itemsToMove.append(myChild);
          myChild = myChild->nextSibling();
        }
        QPtrListIterator<QListViewItem> it( itemsToMove );
        QListViewItem *cur;
        while ((cur = it.current()))
        {
          oldItem->takeItem(cur);
          item->insertItem(cur);
          ++it;
        }
        delete oldItem;
        itemsToMove.clear();
      }

    } else if (onlySubscribed)
    {
      // find the item
      QListViewItem *item = groupView->findItem(mSubfolderPaths[i], 1);
      if (item)
        static_cast<GroupItem*>(item)->setOn(true);
    }
  }
  if ( jobData.inboxOnly ) {
    // list again with prefix
    ai->listDirectory( ai->prefix(), type, true, 0, false, true );
  } else if (onlySubscribed) {
    // activate buttons and stuff
    slotLoadingComplete();
  }
}

//------------------------------------------------------------------------------
void SubscriptionDialog::findParentItem( QString &name, QString &path, QString &parentPath,
    GroupItem **parent, GroupItem **oldItem )
{
  // remove the name (and the separator) from the path to get the parent path
  int start = path.length() - (name.length()+2);
  int length = name.length()+1;
  if (start < 0) start = 0;
  parentPath = path;
  parentPath.remove(start, length);

  if (mDelimiter.isEmpty())
    return;
  
  // find the parent by it's path
  if (path.find(mDelimiter) != -1)
  {
    QListViewItem *possibleParent = groupView->findItem(parentPath, 1);
    if (possibleParent)
      *parent = static_cast<GroupItem*>(possibleParent);
  }

  // now we have to check if the item already exists
  QListViewItem *possibleItem = groupView->findItem(path, 1);
  if (possibleItem)
    *oldItem = static_cast<GroupItem*>(possibleItem);
}

//------------------------------------------------------------------------------
void SubscriptionDialog::slotSave()
{
  if (!account())
    return;
  // subscribe
  QListViewItemIterator it(subView);
  for ( ; it.current(); ++it)
  {
    static_cast<ImapAccountBase*>(account())->changeSubscription(true,
        static_cast<GroupItem*>(it.current())->info().path);
  }

  // unsubscribe
  QListViewItemIterator it2(unsubView);
  for ( ; it2.current(); ++it2)
  {
    static_cast<ImapAccountBase*>(account())->changeSubscription(false,
        static_cast<GroupItem*>(it2.current())->info().path);
  }
}

//------------------------------------------------------------------------------
void SubscriptionDialog::slotLoadFolders()
{
  // clear the views
  KSubscription::slotLoadFolders();
  if ( !account() )
    return;
  ImapAccountBase* ai = static_cast<ImapAccountBase*>(account());
  if ( ai->prefix().isEmpty() )
    return;
  // only do a complete listing (*) when the user did not enter a prefix
  // otherwise the complete listing will be done in second step
  bool complete = (ai->prefix() == "/") ? true : false;
  // get all folders
  ai->listDirectory( ai->prefix(), ImapAccountBase::List, 
      false, 0, true, complete );
  // get subscribed folders
  ai->listDirectory( ai->prefix(), ImapAccountBase::ListSubscribedNoCheck, 
      false, 0, false, complete );
}

//------------------------------------------------------------------------------
void SubscriptionDialog::slotCancel()
{
  if ( account() )
  {
    ImapAccountBase* ai = static_cast<ImapAccountBase*>(account());
    ai->killAllJobs();
  }
  KSubscription::slotCancel();
}

} // namespace

#include "subscriptiondialog.moc"
