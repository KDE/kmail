/*  -*- c++ -*-
    subscriptiondialog.h

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#ifndef __SUBSCRIPTIONDIALOG
#define __SUBSCRIPTIONDIALOG

#include <q3dict.h>
#include <ksubscription.h>
#include "imapaccountbase.h"

class KMMessage;
class FolderStorage;

namespace KMail {

  // Abstract base class for the server side and client side subscription dialogs.
  // Scott Meyers says: "Make non-leaf classes abstract" and he is right, I think.
  // (More Effective C++, Item 33)
  class SubscriptionDialogBase : public KSubscription
  {
    Q_OBJECT

    public:
      SubscriptionDialogBase( QWidget *parent,
                              const QString &caption,
                              KAccount* acct,
                              QString startPath = QString() );
      virtual ~SubscriptionDialogBase() {}

      void show();

    protected:
      /**
       * Find the parent item
       */
      void findParentItem ( QString &name, QString &path, QString &compare,
                       GroupItem **parent, GroupItem **oldItem );

      /**
       * Process the next prefix in mPrefixList
       */
      void processNext();

      /**
       * Fill mPrefixList
       */
      void initPrefixList();

      virtual void loadingComplete();

    public slots:
      /**
       * get the listing from the imap-server
       */
      void slotListDirectory(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData &);

      /**
       * called by Ok-button, saves the changes
       */
      void slotSave();

      /**
       * Called from the account when a connection was established
       */
      void slotConnectionResult( int errorCode, const QString& errorMsg );

    protected slots:
      /**
       * Loads the folders
       */
      void slotLoadFolders();

    protected:
      virtual void listAllAvailableAndCreateItems() = 0;
      virtual void processFolderListing() = 0;
      virtual void doSave() = 0;

      // helpers
      /** Move all child items of @param oldItem under @param item */
      void moveChildrenToNewParent( GroupItem *oldItem, GroupItem *item  );

      /** Create a listview item for the i-th entry in the list of available
       * folders. */
      void createListViewItem( int i );

      QString mDelimiter;
      QStringList mFolderNames, mFolderPaths,
                  mFolderMimeTypes, mFolderAttributes;
      ImapAccountBase::jobData mJobData;
      uint mCount;
      Q3Dict<GroupItem> mItemDict;
      QString mStartPath;
      bool mSubscribed, mForceSubscriptionEnable;
      QStringList mPrefixList;
      QString mCurrentNamespace;
  };

  class SubscriptionDialog : public SubscriptionDialogBase
  {
    Q_OBJECT
    public:

      SubscriptionDialog( QWidget *parent,
                          const QString &caption,
                          KAccount* acct,
                          QString startPath = QString() );
      virtual ~SubscriptionDialog();
     protected:
      /** reimpl */
      virtual void listAllAvailableAndCreateItems();
      /** reimpl */
      virtual void processFolderListing();
      /** reimpl */
      virtual void doSave();

    private:
      /**
       * Create or update the listitems, depending on whether we are listing
       * all available folders, or only subscribed ones.
       */
      void processItems();

  };

} // namespace KMail

#endif
