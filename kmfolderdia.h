// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * kmfolderdia.h
 *
 * Copyright (c) 1997-2004 KMail Developers
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <kdialogbase.h>
#include "configuredialog_p.h"
#include <qvaluevector.h>

class QCheckBox;
class QPushButton;
class QLineEdit;
class QListBox;
class QComboBox;
class QGroupBox;
class KMFolder;
class KMFolderTreeItem;
class KMFolderDir;
class KIntNumInput;
class KIconButton;
class KEditListBox;
namespace KPIM { class IdentityCombo; }
class KMFolderDialog;
class KMFolderTree;
template <typename T> class QGuardedPtr;

namespace KMail {
  class FolderRequester; 
/**
 * This is the base class for tabs in the folder dialog.
 * It uses the API from ConfigModuleTab (basically: it's a widget that can load and save)
 * but it also adds support for delayed-saving:
 * when save() needs to use async jobs (e.g. KIO) for saving,
 * we need to delay the closing until after the jobs are finished,
 * and to cancel the saving on error.
 *
 * Feel free to rename and move this base class somewhere else if it
 * can be useful for other dialogs.
 */
class FolderDiaTab : public QWidget
{
  Q_OBJECT
public:
   FolderDiaTab( QWidget *parent=0, const char* name=0 )
     : QWidget( parent, name ) {}

  virtual void load() = 0;

  /// Unlike ConfigModuleTab, we return a bool from save.
  /// This allows to cancel closing on error.
  /// When called from the Apply button, the return value is ignored
  /// @return whether save succeeded
  virtual bool save() = 0;

  enum AcceptStatus { Accepted, Canceled, Delayed };
  /// Called when clicking OK.
  /// If a module returns Delayed, the closing is cancelled for now,
  /// and the module can close the dialog later on (i.e. after an async
  /// operation like a KIO job).
  virtual AcceptStatus accept() {
    return save() ? Accepted : Canceled;
  }

signals:
  /// Emit this to tell the dialog that you're done with the async jobs,
  /// and that the dialog can be closed.
  void readyForAccept();

  /// Emit this, i.e. after a job had an error, to tell the dialog to cancel
  /// the closing.
  void cancelAccept();

  /// Called when this module was changed [not really used yet]
  void changed(bool);
};

/**
 * "General" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDiaGeneralTab : public FolderDiaTab
{
  Q_OBJECT

public:
  FolderDiaGeneralTab( KMFolderDialog* dlg,
                       const QString& aName,
                       QWidget* parent, const char* name = 0 );

  virtual void load();
  virtual bool save();

private slots:
  void slotChangeIcon( QString icon );
  /*
   * is called if the folder dropdown changes
   * then we update the other items to reflect the capabilities
   */
  void slotUpdateItems( KMFolder* );
  void slotFolderNameChanged( const QString& );
  void slotFolderContentsSelectionChanged( int );

private:
  void initializeWithValuesFromFolder( KMFolder* folder );

private:
  QComboBox *mMailboxTypeComboBox;
  QComboBox *mShowSenderReceiverComboBox;
  QComboBox *mContentsComboBox;
  QGroupBox *mIncidencesForGroup;
  QComboBox *mIncidencesForComboBox;
  QLabel      *mNormalIconLabel;
  KIconButton *mNormalIconButton;
  QLabel      *mUnreadIconLabel;
  KIconButton *mUnreadIconButton;
  QCheckBox   *mIconsCheckBox;
  QCheckBox   *mNewMailCheckBox;
  QCheckBox   *mNotifyOnNewMailCheckBox;
  QCheckBox   *mKeepRepliesInSameFolderCheckBox;
  KLineEdit   *mNameEdit;

  KPIM::IdentityCombo *mIdentityComboBox;
  QGroupBox *mMailboxTypeGroupBox;

  KMFolderDialog* mDlg;
};

} // end of namespace KMail

/**
 * Dialog for handling the properties of a mail folder
 */
class KMFolderDialog : public KDialogBase
{
  Q_OBJECT

public:
  KMFolderDialog( KMFolder *folder, KMFolderDir *aFolderDir,
		  KMFolderTree* parent, const QString& caption,
                  const QString& name = QString::null );

  KMFolder* folder() const { return mFolder; }
  void setFolder( KMFolder* folder );
  // Was mFolder just created? (This only makes sense from save())
  // If Apply is clicked, or OK proceeeds half-way, then next time "new folder" will be false.
  bool isNewFolder() const { return mIsNewFolder; }

  KMFolderDir* folderDir() const { return mFolderDir; }
  typedef QValueList<QGuardedPtr<KMFolder> > FolderList;

  KMFolder* parentFolder() const { return mParentFolder; }

  KMFolderTree* folderTree() const { return mFolderTree; }

protected slots:
  void slotChanged( bool );
  virtual void slotOk();
  virtual void slotApply();

  void slotReadyForAccept();
  void slotCancelAccept();

private:
  void addTab( KMail::FolderDiaTab* tab );

private:
  // Can be 0 initially when creating a folder, but will be set by save() in the first tab.
  QGuardedPtr<KMFolder> mFolder;
  QGuardedPtr<KMFolderDir> mFolderDir;
  QGuardedPtr<KMFolder> mParentFolder;

  bool mIsNewFolder; // if true, save() did set mFolder.

  QValueVector<KMail::FolderDiaTab*> mTabs;
  int mDelayedSavingTabs; // this should go into a base class one day
  KMFolderTree* mFolderTree;
};

#endif /*__KMFOLDERDIA*/

