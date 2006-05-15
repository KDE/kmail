/*  -*- c++ -*-
    localsubscriptiondialog.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (C) 2006 Till Adam <adam@kde.org>

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

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "localsubscriptiondialog.h"
#include "kmkernel.h"
#include "kmacctmgr.h"
#include "kmmessage.h"
#include "imapaccountbase.h"
#include "listjob.h"

namespace KMail {

LocalSubscriptionDialog::LocalSubscriptionDialog( QWidget *parent, const QString &caption,
    ImapAccountBase *acct, QString startPath )
  : SubscriptionDialog( parent, caption, acct, startPath ),
    mAccount( acct )
{
}

/* virtual */
LocalSubscriptionDialog::~LocalSubscriptionDialog()
{

}


/*virtual*/
void LocalSubscriptionDialog::listAllAvailableAndCreateItems()
{
  // only do a complete listing (*) when the user did not enter a prefix
  // otherwise the complete listing will be done in second step
  bool complete = (mAccount->prefix() == "/") ? true : false;
  // get all available folders
  ImapAccountBase::ListType type = ImapAccountBase::List;
  if ( mAccount->onlySubscribedFolders() )
      type = ImapAccountBase::ListSubscribedNoCheck;
  ListJob* job = new ListJob( 0, mAccount, type, false,
                              complete, false, mAccount->prefix() );
  connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
      this, SLOT(slotListDirectory(const QStringList&, const QStringList&,
          const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
  job->start();
}

/* virtual */
void LocalSubscriptionDialog::processFolderListing()
{
  uint done = 0;
  for (uint i = mCount; i < mFolderNames.count(); ++i) {
    // give the dialog a chance to repaint
    if (done == 1000) {
      emit listChanged();
      QTimer::singleShot(0, this, SLOT(processFolderListing()));
      return;
    }
    ++mCount;
    ++done;
    createListViewItem( i );
  }
  if ( mJobData.inboxOnly ) {
    // So far we've only listed the inbox, so list the rest of the
    // available folders and process them. We only get here if there
    // is a custom prefix. (secondStep=true)
    ImapAccountBase::ListType type = ImapAccountBase::List;
    if ( mAccount->onlySubscribedFolders() )
      type = ImapAccountBase::ListSubscribedNoCheck;
    ListJob* job = new ListJob( 0, mAccount, type, true, true,
                                false, mAccount->prefix() );
    connect( job, SIGNAL(receivedFolders(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)),
        this, SLOT(slotListDirectory(const QStringList&, const QStringList&,
            const QStringList&, const QStringList&, const ImapAccountBase::jobData&)));
    job->start();
  } else {
    // We only get here if we have processed all listed folders, either 
    // immediately, if there is no prefix, or after the second listing.
    setCheckedStateOfAllItems();
  }
}

void LocalSubscriptionDialog::setCheckedStateOfAllItems()
{
   // iterate over all items and check them, unless they are
   // in the account's local subscription blacklist
   QDictIterator<GroupItem> it( mItemDict );
   for ( ; it.current(); ++it ) {
     GroupItem *item = it.current();
     QString path = it.currentKey();
     item->setOn( mAccount->locallySubscribedTo( path ) );
   }
   slotLoadingComplete();
}

/*virtual*/
void LocalSubscriptionDialog::doSave()
{
  bool somethingHappened = false;
  // subscribe
  QListViewItemIterator it(subView);
  for ( ; it.current(); ++it) {
    static_cast<ImapAccountBase*>(account())->changeLocalSubscription(
        static_cast<GroupItem*>(it.current())->info().path, true );
    somethingHappened = true;
  }

  // unsubscribe
  QListViewItemIterator it2(unsubView);
  if ( unsubView->childCount() > 0 ) {
    const QString message = i18n("Locally unsubscribing from folders will remove all "
        "information that is present locally about those folders. The folders will " 
        "not be changed on the server. Press cancel now if you want to make sure "
        "all local changes have been written to the server by checking mail first.");
    const QString caption = i18n("Local changes will be lost when unsubscribing");
    if ( KMessageBox::warningContinueCancel( this, message, caption )
        != KMessageBox::Cancel ) {
      somethingHappened = true;
      for ( ; it2.current(); ++it2) {
        static_cast<ImapAccountBase*>(account())->changeLocalSubscription(
            static_cast<GroupItem*>(it2.current())->info().path, false );
      }

    }
  }
  if ( somethingHappened ) {
    kmkernel->acctMgr()->singleCheckMail( mAccount, true);
  }
}

} // namespace

#include "localsubscriptiondialog.moc"
