// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * folderdialogacltab.h
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
#ifndef COLLECTIONACLPAGE_H
#define COLLECTIONACLPAGE_H

#include <akonadi/collectionpropertiespage.h>
#include <kimap/acl.h>
#include <KDialog>
class KJob;
class KPushButton;
class QStackedWidget;
class KHBox;
class QButtonGroup;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;

class KLineEdit;
namespace KIO { class Job; }

enum IMAPUserIdFormat { FullEmail, UserName };

struct ACLListEntry;
typedef QVector<ACLListEntry> ACLList;


/**
 * "New Access Control Entry" dialog.
 * Internal class, only used by CollectionAclPage
 */
class ACLEntryDialog :public KDialog {
  Q_OBJECT

public:
  ACLEntryDialog( const QString& caption, QWidget* parent );
  void setValues( const QString& userId, KIMAP::Acl::Rights permissions );
  QString userId() const;
  QStringList userIds() const;
  KIMAP::Acl::Rights permissions() const;

private slots:
  void slotSelectAddresses();
  void slotChanged();

private:
  QButtonGroup* mButtonGroup;
  KLineEdit* mUserIdLineEdit;
};

/**
 * "Access Control" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class CollectionAclPage : public Akonadi::CollectionPropertiesPage
{
  Q_OBJECT

public:
  CollectionAclPage( QWidget* parent = 0 );
  void load( const Akonadi::Collection & col );
  void save( Akonadi::Collection & col );
protected:
  void init();
private slots:
  // User (QTreeWidget) slots
  void slotEditACL( QTreeWidgetItem* );
  void slotSelectionChanged();

  // User (pushbuttons) slots
  void slotAddACL();
  void slotEditACL();
  void slotRemoveACL();

#if 0
  void slotChanged( bool b );

#endif
private:
  void addACLs( const QStringList& userIds, KIMAP::Acl::Rights permissions );

private:
  // The widget containing the ACL widgets (listview and buttons)
  KHBox* mACLWidget;
  class ListViewItem;

  QTreeWidget* mListView;

  KPushButton* mAddACL;
  KPushButton* mEditACL;
  KPushButton* mRemoveACL;
  QStringList mRemovedACLs;
#if 0
  QString mImapPath;
#endif
  QString mImapUserName;
  KIMAP::Acl::Rights mUserRights;
#if 0
  ACLList mInitialACLList;
  ACLList mACLList; // to be set
  IMAPUserIdFormat mUserIdFormat;
#endif
  QLabel* mLabel;
  QStackedWidget* mStack;

  bool mChanged;
  bool mAccepting; // i.e. close when done
  bool mSaving;
};

#endif /* COLLECTIONACLPAGE_H */

