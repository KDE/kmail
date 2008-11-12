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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_CONFIGURETHEMESDIALOG_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_CONFIGURETHEMESDIALOG_H__

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
class ThemeEditor;
class Theme;
class ThemeListWidgetItem;

class ThemeListWidget : public QListWidget
{
  Q_OBJECT
public:
  ThemeListWidget( QWidget * parent )
    : QListWidget( parent )
    {};
public:
  // need a larger but shorter QListWidget
  QSize sizeHint() const
    { return QSize( 450, 128 ); };
};

class ConfigureThemesDialog : public KDialog
{
  friend class Manager;

  Q_OBJECT
protected:
  ConfigureThemesDialog( QWidget *parent = 0 );
  ~ConfigureThemesDialog();

private:
  static ConfigureThemesDialog * mInstance;
  ThemeListWidget *mThemeList;
  ThemeEditor *mEditor;
  QPushButton *mNewThemeButton;
  QPushButton *mCloneThemeButton;
  QPushButton *mDeleteThemeButton;

protected:
  static void display( QWidget * parent, const QString &preselectThemeId = QString() );
  static void cleanup();

public:
  static ConfigureThemesDialog * instance()
    { return mInstance; };

private:
  void fillThemeList();
  QString uniqueNameForTheme( QString baseName, Theme * skipTheme = 0 );
  ThemeListWidgetItem * findThemeItemByName( const QString &name, Theme * skipTheme = 0 );
  ThemeListWidgetItem * findThemeItemByTheme( Theme * set );
  ThemeListWidgetItem * findThemeItemById( const QString &themeId );
  void selectThemeById( const QString &themeId );
  void commitEditor();

private slots:
  void themeListCurrentItemChanged( QListWidgetItem * cur, QListWidgetItem * prev );
  void newThemeButtonClicked();
  void cloneThemeButtonClicked();
  void deleteThemeButtonClicked();
  void editedThemeNameChanged();
  void okButtonClicked();
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_CONFIGURESKINSDIALOG_H__
