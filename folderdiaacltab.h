// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * folderdiaacltab.h
 *
 * Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef FOLDERDIAACL_H
#define FOLDERDIAACL_H

#include "kmfolderdia.h"
#include "kmfoldertype.h"

class KMFolderImap;
class KPushButton;
class QWidgetStack;
class QHBox;
class QVButtonGroup;
class KListView;
namespace KIO { class Job; }

namespace KMail {

enum IMAPUserIdFormat { FullEmail, UserName };

struct ACLListEntry;
typedef QValueVector<KMail::ACLListEntry> ACLList;

class ImapAccountBase;

/**
 * "New Access Control Entry" dialog.
 * Internal class, only used by FolderDiaACLTab
 */
class ACLEntryDialog :public KDialogBase {
  Q_OBJECT

public:
  ACLEntryDialog( IMAPUserIdFormat userIdFormat, const QString& caption, QWidget* parent, const char* name = 0 );

  void setValues( const QString& userId, unsigned int permissions );

  QString userId() const;
  QStringList userIds() const;
  unsigned int permissions() const;

private slots:
  void slotSelectAddresses();
  void slotChanged();

private:
  QVButtonGroup* mButtonGroup;
  KLineEdit* mUserIdLineEdit;
  IMAPUserIdFormat mUserIdFormat;
};

/**
 * "Access Control" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDiaACLTab : public FolderDiaTab
{
  Q_OBJECT

public:
  FolderDiaACLTab( KMFolderDialog* dlg, QWidget* parent, const char* name = 0 );

  virtual void load();
  virtual bool save();
  virtual AcceptStatus accept();

  static bool supports( KMFolder* refFolder );

private slots:
  // Network (KIO) slots
  void slotConnectionResult( int, const QString& );
  void slotReceivedACL( KMFolder*, KIO::Job*, const KMail::ACLList& );
  void slotMultiSetACLResult(KIO::Job *);
  void slotACLChanged( const QString&, int );
  void slotReceivedUserRights( KMFolder* folder );
  void slotDirectoryListingFinished(KMFolderImap*);

  // User (KListView) slots
  void slotEditACL(QListViewItem*);
  void slotSelectionChanged(QListViewItem*);

  // User (pushbuttons) slots
  void slotAddACL();
  void slotEditACL();
  void slotRemoveACL();

  void slotChanged( bool b );

private:
  KURL imapURL() const;
  void initializeWithValuesFromFolder( KMFolder* folder );
  void startListing();
  void loadListView( const KMail::ACLList& aclList );
  void loadFinished( const KMail::ACLList& aclList );
  void addACLs( const QStringList& userIds, unsigned int permissions );

private:
  // The widget containing the ACL widgets (listview and buttons)
  QHBox* mACLWidget;
  //class ListView;
  class ListViewItem;
  KListView* mListView;
  KPushButton* mAddACL;
  KPushButton* mEditACL;
  KPushButton* mRemoveACL;

  QStringList mRemovedACLs;
  QString mImapPath;
  ImapAccountBase* mImapAccount;
  int mUserRights;
  KMFolderType mFolderType;
  ACLList mInitialACLList;
  ACLList mACLList; // to be set
  IMAPUserIdFormat mUserIdFormat;

  QLabel* mLabel;
  QWidgetStack* mStack;
  KMFolderDialog* mDlg;

  bool mChanged;
  bool mAccepting; // i.e. close when done
  bool mSaving;
};

} // end of namespace KMail

#endif /* FOLDERDIAACL_H */

