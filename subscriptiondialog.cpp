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
#include "folderstorage.h"
#include "listjob.h"
using KMail::ListJob;

#include <klocale.h>
#include <kdebug.h>


namespace KMail {

SubscriptionDialog::SubscriptionDialog( QWidget *parent, const QString &caption,
    KAccount *acct, QString startPath )
  : KSubscription( parent, caption, acct, User1, QString::null, false ),
    mStartPath( startPath )
{
  // hide unneeded checkboxes
  hideTreeCheckbox();
  hideNewOnlyCheckbox();

  // ok-button
  connect(this, SIGNAL(okClicked()), SLOT(slotSave()));

  // reload-list button
  connect(this, SIGNAL(user1Clicked()), SLOT(slotLoadFolders()));

  // get the folders
  slotLoadFolders();
}

//------------------------------------------------------------------------------
void SubscriptionDialog::slotListDirectory( const QStringList& subfolderNames,
                                            const QStringList& subfolderPaths,
                                            const QStringList& subfolderMimeTypes,
                                            const QStringList& subfolderAttributes,
                                            const ImapAccountBase::jobData& jobData )
{
  mFolderNames = subfolderNames;
  mFolderPaths = subfolderPaths;
  mFolderMimeTypes = subfolderMimeTypes;
  mFolderAttributes = subfolderAttributes;
  mJobData = jobData;

  mCount = 0;
  mCheckForExisting = false;

  createItems();
}

//------------------------------------------------------------------------------
void SubscriptionDialog::createItems()
{
  bool onlySubscribed = mJobData.onlySubscribed;
  ImapAccountBase* ai = static_cast<ImapAccountBase*>(mAcct);
  GroupItem *parent = 0;
  uint done = 0;
  for (uint i = mCount; i < mFolderNames.count(); ++i)
  {
    // give the dialog a chance to repaint
    if (done == 1000)
    {
      emit listChanged();
      QTimer::singleShot(0, this, SLOT(createItems()));
      return;
    }
    ++mCount;
    ++done;
    GroupItem *item = 0;
    if (!onlySubscribed && mFolderPaths.size() > 0)
    {
      // the account does not know the delimiter
      if (mDelimiter.isEmpty())
      {
        int start = mFolderPaths[i].findRev(mFolderNames[i]);
        if (start > 1)
          mDelimiter = mFolderPaths[i].mid(start-1, 1);
      }

      // get the parent
      GroupItem *oldItem = 0;
      QString parentPath;
      findParentItem( mFolderNames[i], mFolderPaths[i], parentPath, &parent, &oldItem );

      if (!parent && parentPath != "/")
      {
        // the parent is not available and it's no root-item
        // this happens when the folders do not arrive in hierarchical order
        // so we create each parent in advance
        // as a result we have to check from now on if the folder already exists
        mCheckForExisting = true;
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
          item = 0;
          if (folders.count() > 1)
          {
            // we have to create more then one level, so better check if this
            // folder already exists somewhere
            item = mItemDict[path];
          }
          // as these items are "dummies" we create them non-checkable
          if (!item)
          {
            if (parent)
              item = new GroupItem(parent, info, this, false);
            else
              item = new GroupItem(folderTree(), info, this, false);
            mItemDict.insert(info.path, item);
          }

          parent = item;
          ++i;
        } // folders
      } // parent
    
      KGroupInfo info(mFolderNames[i]);
      if (mFolderNames[i].upper() == "INBOX" &&
          mFolderPaths[i] == "/INBOX/")
        info.name = i18n("inbox");
      info.subscribed = false;
      info.path = mFolderPaths[i];
      // only checkable when the folder is selectable
      bool checkable = ( mFolderMimeTypes[i] == "inode/directory" ) ? false : true;
      // create a new item
      if (parent)
        item = new GroupItem(parent, info, this, checkable);
      else
        item = new GroupItem(folderTree(), info, this, checkable);

      if (oldItem) // remove old item
        mItemDict.remove(info.path);
      
      mItemDict.insert(info.path, item);
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
          if ( cur->isSelected() ) // we have new parents so open them
            folderTree()->ensureItemVisible( cur );
          ++it;
        }
        delete oldItem;
        itemsToMove.clear();
      }
      // select the start item
      if ( mFolderPaths[i] == mStartPath )
      {
        item->setSelected( true );
        folderTree()->ensureItemVisible( item );
      }

    } else if (onlySubscribed)
    {
      // find the item
      if ( mItemDict[mFolderPaths[i]] )
      {
        GroupItem* item = mItemDict[mFolderPaths[i]];
        item->setOn( true );
      }
    }
  }
  if ( mJobData.inboxOnly ) 
  {
    // list again (secondStep=true) with prefix
    ImapAccountBase::ListType type = ImapAccountBase::List;
    if ( onlySubscribed )
      type = ImapAccountBase::ListSubscribedNoCheck;
    ListJob* job = new ListJob( 0, ai, type, true, true,
        false, ai->prefix() );
    connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
        this, SLOT(slotListDirectory(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
    job->start();
  } else if (!onlySubscribed) 
  {
    // get subscribed folders
    // only do a complete listing (*) when the user did not enter a prefix
    // otherwise the complete listing will be done in second step
    // we use the 'SubscribedNoCheck' feature so that the kioslave doesn't check
    // for every folder if it actually exists
    bool complete = (ai->prefix() == "/") ? true : false;
    ListJob* job = new ListJob( 0, ai, ImapAccountBase::ListSubscribedNoCheck,
        false, complete, false, ai->prefix() );
    connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
        this, SLOT(slotListDirectory(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
    job->start();
  } else if (onlySubscribed)
  {
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
  *parent = mItemDict[parentPath];
  
  // check if the item already exists
  if (mCheckForExisting) 
    *oldItem = mItemDict[path];
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
  mItemDict.clear();
  // only do a complete listing (*) when the user did not enter a prefix
  // otherwise the complete listing will be done in second step
  bool complete = (ai->prefix() == "/") ? true : false;
  // get all folders
  ListJob* job = new ListJob( 0, ai, ImapAccountBase::List, false,
      complete, false, ai->prefix() );
  connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
      this, SLOT(slotListDirectory(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
  job->start();
}

} // namespace

#include "subscriptiondialog.moc"
