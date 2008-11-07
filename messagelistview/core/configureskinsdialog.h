/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_CONFIGURESKINSDIALOG_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_CONFIGURESKINSDIALOG_H__

#include <KDialog>

#include <QListWidget>

class QPushButton;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class Manager;
class SkinEditor;
class Skin;
class SkinListWidgetItem;

class SkinListWidget : public QListWidget
{
  Q_OBJECT
public:
  SkinListWidget( QWidget * parent )
    : QListWidget( parent )
    {};
public:
  // need a larger but shorter QListWidget
  QSize sizeHint() const
    { return QSize( 450, 128 ); };
};

class ConfigureSkinsDialog : public KDialog
{
  friend class Manager;

  Q_OBJECT
protected:
  ConfigureSkinsDialog( QWidget *parent = 0 );
  ~ConfigureSkinsDialog();

private:
  static ConfigureSkinsDialog * mInstance;
  SkinListWidget *mSkinList;
  SkinEditor *mEditor;
  QPushButton *mNewSkinButton;
  QPushButton *mCloneSkinButton;
  QPushButton *mDeleteSkinButton;

protected:
  static void display( QWidget * parent, const QString &preselectSkinId = QString() );
  static void cleanup();

public:
  static ConfigureSkinsDialog * instance()
    { return mInstance; };

private:
  void fillSkinList();
  QString uniqueNameForSkin( QString baseName, Skin * skipSkin = 0 );
  SkinListWidgetItem * findSkinItemByName( const QString &name, Skin * skipSkin = 0 );
  SkinListWidgetItem * findSkinItemBySkin( Skin * set );
  SkinListWidgetItem * findSkinItemById( const QString &skinId );
  void selectSkinById( const QString &skinId );
  void commitEditor();

private slots:
  void skinListCurrentItemChanged( QListWidgetItem * cur, QListWidgetItem * prev );
  void newSkinButtonClicked();
  void cloneSkinButtonClicked();
  void deleteSkinButtonClicked();
  void editedSkinNameChanged();
  void okButtonClicked();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_CONFIGURESKINSDIALOG_H__
