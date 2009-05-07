// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * kmfolderdialog.h
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
#ifndef __KMFOLDERDIALOG_H
#define __KMFOLDERDIALOG_H

#include <kpagedialog.h>
#include "configuredialog_p.h"

#include "kmmainwidget.h"

#include <QList>

class QCheckBox;
class QLabel;
class KComboBox;
class KMFolder;
class KMFolderDir;
class KIconButton;
namespace KPIMIdentities { class IdentityCombo; }
class KMFolderDialog;
template <typename T> class QPointer;

class TemplatesConfiguration;
class KPushButton;

namespace KMail {

class MainFolderView;

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
class FolderDialogTab : public QWidget
{
  Q_OBJECT
public:
   explicit FolderDialogTab( KMFolderDialog *dlg,
                             QWidget *parent = 0,
                             const char *name = 0 )
     : QWidget( parent ),
       mDlg( dlg )
   {
     setObjectName( name );
   }

  virtual void load() = 0;

  /// Unlike ConfigModuleTab, we return a bool from save.
  /// This allows to cancel closing on error.
  /// When called from the Apply button, the return value is ignored
  /// @return whether save succeeded
  virtual bool save() = 0;

  enum AcceptStatus { Accepted, Canceled, Delayed };
  /// Called when clicking OK.
  /// If a module returns Delayed, the closing is canceled for now,
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

protected:
  KMFolderDialog *mDlg;
};

/**
 * "General" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDialogGeneralTab : public FolderDialogTab
{
  Q_OBJECT

public:
  FolderDialogGeneralTab( KMFolderDialog* dlg,
                          const QString& aName,
                          QWidget* parent, const char* name = 0 );

  virtual void load();
  virtual bool save();

private slots:
  void slotChangeIcon( const QString &icon );
  /*
   * is called if the folder dropdown changes
   * then we update the other items to reflect the capabilities
   */
  void slotFolderNameChanged( const QString& );
  void slotFolderContentsSelectionChanged( int );
  void slotIdentityCheckboxChanged();

private:
  void initializeWithValuesFromFolder( KMFolder* folder );

private:
  KComboBox *mShowSenderReceiverComboBox;
  KComboBox *mContentsComboBox;
  KComboBox *mIncidencesForComboBox;
  QCheckBox *mAlarmsBlockedCheckBox;
  QCheckBox *mSharedSeenFlagsCheckBox;
  QLabel      *mNormalIconLabel;
  KIconButton *mNormalIconButton;
  QLabel      *mUnreadIconLabel;
  KIconButton *mUnreadIconButton;
  QCheckBox   *mIconsCheckBox;
  QCheckBox   *mNewMailCheckBox;
  QCheckBox   *mNotifyOnNewMailCheckBox;
  QCheckBox   *mKeepRepliesInSameFolderCheckBox;
  QCheckBox   *mUseDefaultIdentityCheckBox;
  KLineEdit   *mNameEdit;

  KPIMIdentities::IdentityCombo *mIdentityComboBox;

  bool mIsLocalSystemFolder;
  bool mIsResourceFolder;
};


 /**
 * "Maintenance" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDialogMaintenanceTab : public FolderDialogTab
{
  Q_OBJECT

public:
  FolderDialogMaintenanceTab( KMFolderDialog *dlg, QWidget *parent );

  virtual void load();
  virtual bool save();

private slots:
  void slotCompactNow();
  void slotRebuildIndex();
  void slotRebuildImap();
  void updateFolderIndexSizes();

private:
  void updateControls();

private:
  KMFolder* mFolder;

  QLabel *mFolderSizeLabel;
  QLabel *mIndexSizeLabel;
  QPushButton *mRebuildIndexButton;
  QPushButton *mRebuildImapButton;
  QLabel *mCompactStatusLabel;
  QPushButton *mCompactNowButton;
};


/**
 * "Templates" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDialogTemplatesTab : public FolderDialogTab
{
  Q_OBJECT

public:
  FolderDialogTemplatesTab( KMFolderDialog *dlg, QWidget *parent );

  virtual void load();
  virtual bool save();

public slots:
  void slotEmitChanged(); // do nothing for now

  void slotCopyGlobal();

private:
  void initializeWithValuesFromFolder( KMFolder* folder );

private:
  QCheckBox* mCustom;
  TemplatesConfiguration* mWidget;
  KPushButton* mCopyGlobal;
  KMFolder* mFolder;
  uint mIdentity;

  bool mIsLocalSystemFolder;
};

} // end of namespace KMail

/**
 * Dialog for handling the properties of a mail folder
 */
class KMFolderDialog : public KPageDialog
{
  Q_OBJECT

public:
  KMFolderDialog( KMFolder *folder, KMFolderDir *aFolderDir,
                  KMail::MainFolderView *parent, const QString& caption,
                  const QString& name = QString() );

  KMFolder* folder() const { return mFolder; }
  void setFolder( KMFolder* folder );
  // Was mFolder just created? (This only makes sense from save())
  // If Apply is clicked, or OK proceeeds half-way, then next time "new folder" will be false.
  bool isNewFolder() const { return mIsNewFolder; }

  KMFolderDir* folderDir() const;
  typedef QList<QPointer<KMFolder> > FolderList;

  KMFolder* parentFolder() const { return mParentFolder; }

  bool setPage( KMMainWidget::PropsPage whichPage );

protected slots:
  void slotChanged( bool );
  virtual void slotOk();
  virtual void slotApply();

  void slotReadyForAccept();
  void slotCancelAccept();

private:
  void addTab( KMail::FolderDialogTab* tab );

private:
  // Can be 0 initially when creating a folder, but will be set by save() in the first tab.
  QPointer<KMFolder> mFolder;
  QPointer<KMFolderDir> mFolderDir;
  QPointer<KMFolder> mParentFolder;

  bool mIsNewFolder; // if true, save() did set mFolder.

  KPageWidgetItem *mShortcutTab;
  KPageWidgetItem *mExpiryTab;
  KPageWidgetItem *mMaillistTab;
  KPageWidgetItem *mMaintenanceTab;

  QVector<KMail::FolderDialogTab*> mTabs;
  int mDelayedSavingTabs; // this should go into a base class one day
};

#endif /*__KMFOLDERDIALOG_H*/

