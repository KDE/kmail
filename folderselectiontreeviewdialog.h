/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2009 Montel Laurent <montel@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef FOLDERSELECTIONTREEVIEWDIALOG_H
#define FOLDERSELECTIONTREEVIEWDIALOG_H

#include <QAbstractItemView>
#include <KDialog>
#include <akonadi/collection.h>
class KJob;
class FolderSelectionTreeView;

class FolderSelectionTreeViewDialog : public KDialog
{
  Q_OBJECT
public:
  FolderSelectionTreeViewDialog( QWidget *parent, bool enableCheck = true, bool showUnreadCount = false );
  ~FolderSelectionTreeViewDialog();

  void setSelectionMode( QAbstractItemView::SelectionMode mode );
  QAbstractItemView::SelectionMode selectionMode() const;

  Akonadi::Collection selectedCollection() const;
  void setSelectedCollection( const Akonadi::Collection &collection );

  Akonadi::Collection::List selectedCollections() const;


private slots:
  void slotSelectionChanged();
  void slotAddChildFolder();
  void collectionCreationResult(KJob*);

protected:
  void readConfig();
  void writeConfig();
  bool canCreateCollection( Akonadi::Collection & parentCol );

private:
  FolderSelectionTreeView *treeview;
};


#endif /* FOLDERSELECTIONTREEVIEWDIALOG_H */

