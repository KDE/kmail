/*
    subscriptiondialog.cpp

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, US
*/

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
  if (mSubfolderPaths.size() <= 0) return;
  bool onlySubscribed = jobData.onlySubscribed;
  GroupItem *parent = 0;
  mLoading = true;

  if (!onlySubscribed)
  {
    /* we need to find our root-item
       remove the name (and the separator) from the path */
    int start = mSubfolderPaths[0].length() - (mSubfolderNames[0].length()+2);
    int length = mSubfolderNames[0].length()+1;
    if (start < 0) start = 0;
    QString compare = mSubfolderPaths[0];
    compare.remove(start, length);

    QListViewItemIterator it( groupView );
    GroupItem *tmp;
    while ( it.current() != 0 )
    {
      // compare it with each item to find the current root
      tmp = static_cast<GroupItem*>(it.current());
      if (tmp->info().path == compare) {
        parent = tmp;
        break;
      }
      ++it;
    }
  }
  GroupItem *item = 0;
  QString text;
  for (uint i = 0; i < mSubfolderNames.count(); i++)
  {
    if (!onlySubscribed)
    {
      bool create = true;
      if (parent)
      {
        for ( QListViewItem *it = parent->firstChild() ;
            it ; it = it->nextSibling() )
        {
          // check if this item already exists in this hierarchy
          item = static_cast<GroupItem*>(it);
          if (item->info().path == mSubfolderPaths[i])
            create = false;
        }
      }
      if (create)
      {
        KGroupInfo info(mSubfolderNames[i]);
        if (mSubfolderNames[i].upper() == "INBOX")
          info.name = i18n("inbox");
        info.subscribed = false;
        info.path = mSubfolderPaths[i];
        // create a new checkable item
        if (parent)
          item = new GroupItem(parent, info, this, true);
        else
          item = new GroupItem(folderTree(), info, this, true);
      }
      if (item)
      {
        // reset
        item->setOn(false);
      }
    } else {
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
    if (mSubfolderMimeTypes[i] == "message/directory" ||
        mSubfolderMimeTypes[i] == "inode/directory")
    {
      // descend
      static_cast<ImapAccountBase*>(mAcct)->listDirectory(mSubfolderPaths[i],
          onlySubscribed);
    }
  }
  if (jobData.inboxOnly)
  {
    ImapAccountBase* ai = static_cast<ImapAccountBase*>(mAcct);
    ai->listDirectory(ai->prefix(), false, true);
    ai->listDirectory(ai->prefix(), true, true);
  }
  slotLoadingComplete();
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
  KSubscription::slotLoadFolders();
  if ( !account())
    return;
  ImapAccountBase* ai = static_cast<ImapAccountBase*>(account());
  // get folders
  ai->listDirectory(ai->prefix(), false);
  ai->listDirectory(ai->prefix(), true);
}

}

#include "subscriptiondialog.moc"
