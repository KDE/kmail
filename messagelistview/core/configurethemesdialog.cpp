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

#include "messagelistview/core/configurethemesdialog.h"

#include "messagelistview/core/themeeditor.h"
#include "messagelistview/core/theme.h"

#include "messagelistview/core/manager.h"

#include <QGridLayout>
#include <QPushButton>
#include <QFrame>
#include <QHash>

#include <KLocale>
#include <KIconLoader>
#include <KMessageBox>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class ThemeListWidgetItem : public QListWidgetItem
{
private:
  Theme * mTheme;

public:
  ThemeListWidgetItem( QListWidget * par, const Theme &set )
    : QListWidgetItem( set.name(), par )
  {
    mTheme = new Theme( set );
  }
  ~ThemeListWidgetItem()
  {
    if ( mTheme )
      delete mTheme;
  }

public:
  Theme * theme() const
    { return mTheme; };
  void forgetTheme()
    { mTheme = 0; };
};

ConfigureThemesDialog * ConfigureThemesDialog::mInstance = 0;

ConfigureThemesDialog::ConfigureThemesDialog( QWidget *parent )
  : KDialog( parent )
{
  mInstance = this;
  setAttribute( Qt::WA_DeleteOnClose );
  setWindowModality( Qt::ApplicationModal ); // FIXME: Sure ?
  setButtons( Ok | Cancel );
  setWindowTitle( i18n( "Customize Themes" ) );

  QWidget * base = new QWidget( this );
  setMainWidget( base );

  QGridLayout * g = new QGridLayout( base );

  mThemeList = new ThemeListWidget( base );
  mThemeList->setSortingEnabled( true );
  g->addWidget( mThemeList, 0, 0, 5, 1 );

  connect( mThemeList, SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ),
           SLOT( themeListCurrentItemChanged( QListWidgetItem *, QListWidgetItem * ) ) );

  mNewThemeButton = new QPushButton( i18n( "New Theme" ), base );
  mNewThemeButton->setIcon( KIcon( "document-new" ) );
  mNewThemeButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mNewThemeButton, 0, 1 );

  connect( mNewThemeButton, SIGNAL( clicked() ),
           SLOT( newThemeButtonClicked() ) );

  mCloneThemeButton = new QPushButton( i18n( "Clone Theme" ), base );
  mCloneThemeButton->setIcon( KIcon( "edit-copy" ) );
  mCloneThemeButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mCloneThemeButton, 1, 1 );

  connect( mCloneThemeButton, SIGNAL( clicked() ),
           SLOT( cloneThemeButtonClicked() ) );

  QFrame * f = new QFrame( base );
  f->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  f->setMinimumHeight( 24 );
  g->addWidget( f, 2, 1, Qt::AlignVCenter );

  mDeleteThemeButton = new QPushButton( i18n( "Delete Theme" ), base );
  mDeleteThemeButton->setIcon( KIcon( "edit-delete" ) );
  mDeleteThemeButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mDeleteThemeButton, 3, 1 );

  connect( mDeleteThemeButton, SIGNAL( clicked() ),
           SLOT( deleteThemeButtonClicked() ) );

  mEditor = new ThemeEditor( base );
  g->addWidget( mEditor, 5, 0, 1, 2 );

  connect( mEditor, SIGNAL( themeNameChanged() ),
           SLOT( editedThemeNameChanged() ) );

  g->setColumnStretch( 0, 1 );
  g->setRowStretch( 4, 1 );

  connect( this, SIGNAL( okClicked() ),
           SLOT( okButtonClicked() ) );

  fillThemeList();
}

ConfigureThemesDialog::~ConfigureThemesDialog()
{
  mInstance = 0;
}

void ConfigureThemesDialog::display( QWidget *parent, const QString &preselectThemeId )
{
  if ( mInstance )
    return; // FIXME: reparent if needed ?
  mInstance = new ConfigureThemesDialog( parent );
  mInstance->selectThemeById( preselectThemeId );
  mInstance->show();
}

void ConfigureThemesDialog::cleanup()
{
  if ( !mInstance )
    return;
  delete mInstance;
  mInstance = 0;
}

void ConfigureThemesDialog::selectThemeById( const QString &themeId )
{
  ThemeListWidgetItem * item = findThemeItemById( themeId );
  if ( !item )
    return;
  mThemeList->setCurrentItem( item );
}

void ConfigureThemesDialog::okButtonClicked()
{
  commitEditor();

  Manager::instance()->removeAllThemes();

  int c = mThemeList->count();
  int i = 0;
  while ( i < c )
  {
    ThemeListWidgetItem * item = dynamic_cast< ThemeListWidgetItem * >( mThemeList->item( i ) );
    if ( item )
    {
      Manager::instance()->addTheme( item->theme() );
      item->forgetTheme();
    }
    i++;
  }

  Manager::instance()->themesConfigurationCompleted();

  close(); // this will delete too
}

void ConfigureThemesDialog::commitEditor()
{
  Theme * editedTheme = mEditor->editedTheme();
  if ( !editedTheme )
    return;

  mEditor->commit();

  ThemeListWidgetItem * editedItem = findThemeItemByTheme( editedTheme );
  if ( editedItem )
    return;
  QString goodName = uniqueNameForTheme( editedTheme->name(), editedTheme );
  editedTheme->setName( goodName );
  editedItem->setText( goodName );
}

void ConfigureThemesDialog::editedThemeNameChanged()
{
  Theme * set = mEditor->editedTheme();
  if ( !set )
    return;

  ThemeListWidgetItem * it = findThemeItemByTheme( set );
  if ( !it )
    return;

  QString goodName = uniqueNameForTheme( set->name(), set );

  it->setText( goodName );
}

void ConfigureThemesDialog::fillThemeList()
{
  const QHash< QString, Theme * > & sets = Manager::instance()->themes();
  for( QHash< QString, Theme * >::ConstIterator it = sets.begin(); it != sets.end(); ++it )
    (void)new ThemeListWidgetItem( mThemeList, *( *it ) );
}

void ConfigureThemesDialog::themeListCurrentItemChanged( QListWidgetItem * cur, QListWidgetItem * )
{
  commitEditor();

  ThemeListWidgetItem * item = cur ? dynamic_cast< ThemeListWidgetItem * >( cur ) : 0;
  mDeleteThemeButton->setEnabled( item && ( mThemeList->count() > 1 ) );
  mCloneThemeButton->setEnabled( item );
  mEditor->editTheme( item ? item->theme() : 0 );

  if ( item && !item->isSelected() )
    item->setSelected( true ); // make sure it's true
}

ThemeListWidgetItem * ConfigureThemesDialog::findThemeItemById( const QString &themeId )
{
  int c = mThemeList->count();
  int i = 0;
  while ( i < c )
  {
    ThemeListWidgetItem * item = dynamic_cast< ThemeListWidgetItem * >( mThemeList->item( i ) );
    if ( item )
    {
      if ( item->theme()->id() == themeId )
        return item;
    }
    i++;
  }
  return 0;
}


ThemeListWidgetItem * ConfigureThemesDialog::findThemeItemByName( const QString &name, Theme * skipTheme )
{
  int c = mThemeList->count();
  int i = 0;
  while ( i < c )
  {
    ThemeListWidgetItem * item = dynamic_cast< ThemeListWidgetItem * >( mThemeList->item( i ) );
    if ( item )
    {
      if ( item->theme() != skipTheme )
      {
        if ( item->theme()->name() == name )
          return item;
      }
    }
    i++;
  }
  return 0;
}

ThemeListWidgetItem * ConfigureThemesDialog::findThemeItemByTheme( Theme * set )
{
  int c = mThemeList->count();
  int i = 0;
  while ( i < c )
  {
    ThemeListWidgetItem * item = dynamic_cast< ThemeListWidgetItem * >( mThemeList->item( i ) );
    if ( item )
    {
      if ( item->theme() == set )
        return item;
    }
    i++;
  }
  return 0;
}


QString ConfigureThemesDialog::uniqueNameForTheme( QString baseName, Theme * skipTheme )
{
  QString ret = baseName;
  if( ret.isEmpty() )
    ret = i18n( "Unnamed Theme" );

  int idx = 1;

  ThemeListWidgetItem * item = findThemeItemByName( ret, skipTheme );
  while ( item )
  {
    idx++;
    ret = QString("%1 %2").arg( baseName ).arg( idx );
    item = findThemeItemByName( ret, skipTheme );
  }
  return ret;
}

void ConfigureThemesDialog::newThemeButtonClicked()
{
  Theme emptyTheme;
  emptyTheme.setName( uniqueNameForTheme( i18n( "New Theme" ) ) );
  Theme::Column * col = new Theme::Column();
  col->setLabel( i18n( "New Column" ) );
  col->setVisibleByDefault( true );
  col->addMessageRow( new Theme::Row() );
  col->addGroupHeaderRow( new Theme::Row() );
  emptyTheme.addColumn( col );
  ThemeListWidgetItem * item = new ThemeListWidgetItem( mThemeList, emptyTheme );

  mThemeList->setCurrentItem( item );
}

void ConfigureThemesDialog::cloneThemeButtonClicked()
{
  ThemeListWidgetItem * item = dynamic_cast< ThemeListWidgetItem * >( mThemeList->currentItem() );
  if ( !item )
    return;

  Theme copyTheme( *( item->theme() ) );
  copyTheme.detach(); // detach shared data
  copyTheme.generateUniqueId(); // regenerate id so it becomes different
  copyTheme.setName( uniqueNameForTheme( item->theme()->name() ) );
  item = new ThemeListWidgetItem( mThemeList, copyTheme );

  mThemeList->setCurrentItem( item );

}

void ConfigureThemesDialog::deleteThemeButtonClicked()
{
  ThemeListWidgetItem * item = dynamic_cast< ThemeListWidgetItem * >( mThemeList->currentItem() );
  if ( !item )
    return;
  if ( mThemeList->count() < 2 )
    return; // no way: desperately try to keep at least one option set alive :)

  mEditor->editTheme( 0 ); // forget it

  delete item; // this will trigger themeListCurrentItemChanged()
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

