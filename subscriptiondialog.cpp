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
  : KSubscription( parent, caption, acct, User1, QString::null, false )
{
  // hide unneeded checkboxes
  hideTreeCheckbox();
  hideNewOnlyCheckbox();

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
  mLoading = true;
  ImapAccountBase* ai = static_cast<ImapAccountBase*>(mAcct);
  QString text;
  for (uint i = 0; i < mSubfolderNames.count(); i++)
  {
    GroupItem *item = 0;
    if (!onlySubscribed && mSubfolderPaths.size() > 0)
    {
      GroupItem *parent = 0;
      // we need to find our root-item
      // remove the name (and the separator) from the path 
      int start = mSubfolderPaths[i].length() - (mSubfolderNames[i].length()+2);
      int length = mSubfolderNames[i].length()+1;
      if (start < 0) start = 0;
      QString compare = mSubfolderPaths[i];
      compare.remove(start, length);

      QListViewItemIterator it( groupView );
      GroupItem *tmp = 0;
      GroupItem *oldItem = 0;
      bool create = true;
      while ( it.current() != 0 )
      {
        // compare it with each item to find the current root
        tmp = static_cast<GroupItem*>(it.current());
        if (tmp->info().path == mSubfolderPaths[i]) {
          // this item already exists
          create = false;
          oldItem = tmp;
        }
        if (tmp->info().path == compare) {
          parent = tmp;
        }
        ++it;
      }
      if (!parent && compare != "/")
      {
        // the parent is not available
        // this happens when the folders do not arrive in hierarchical order

        // the account does not know the delimiter so we have to reconstruct it
        // we assume that is has length 1
        int start = mSubfolderPaths[i].findRev(mSubfolderNames[i]);
        QString delimiter = mSubfolderPaths[i].mid(start-1, 1);
        // create each parent in advance
        QStringList folders = QStringList::split(delimiter, compare);
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
          QString path = tmpPath.join(delimiter);
          if (!path.startsWith("/"))
            path = "/" + path;
          if (!path.endsWith("/"))
            path = path + "/";
          info.path = path;
          item = 0;
          if (folders.count() > 1)
          {
            // we have to create more then one level, so better check if this
            // folder already exists somewhere
            QListViewItemIterator it( groupView );
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
      // only checkable when the folder is selectable
      bool checkable = ( mSubfolderMimeTypes[i] == "inode/directory" ) ? false : true;
      // create a new item
      if (parent)
        item = new GroupItem(parent, info, this, checkable);
      else
        item = new GroupItem(folderTree(), info, this, checkable);

      if (!create && oldItem)
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
      QListViewItemIterator it( groupView );
      while ( it.current() != 0 )
      {
        item = static_cast<GroupItem*>(it.current());
        if (item->info().path == mSubfolderPaths[i])
        {
          // subscribed
          item->setOn(true);
        }
        ++it;
      }
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
