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

#include "messagelistview/core/configureskinsdialog.h"

#include "messagelistview/core/skineditor.h"
#include "messagelistview/core/skin.h"

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

class SkinListWidgetItem : public QListWidgetItem
{
private:
  Skin * mSkin;

public:
  SkinListWidgetItem( QListWidget * par, const Skin &set )
    : QListWidgetItem( set.name(), par )
  {
    mSkin = new Skin( set );
  }
  ~SkinListWidgetItem()
  {
    if ( mSkin )
      delete mSkin;
  }

public:
  Skin * skin() const
    { return mSkin; };
  void forgetSkin()
    { mSkin = 0; };
};

ConfigureSkinsDialog * ConfigureSkinsDialog::mInstance = 0;

ConfigureSkinsDialog::ConfigureSkinsDialog( QWidget *parent )
  : KDialog( parent )
{
  mInstance = this;
  setAttribute( Qt::WA_DeleteOnClose );
  setWindowModality( Qt::ApplicationModal ); // FIXME: Sure ?
  setButtons( Ok | Cancel );
  setWindowTitle( i18n( "Customize Skins" ) );

  QWidget * base = new QWidget( this );
  setMainWidget( base );

  QGridLayout * g = new QGridLayout( base );

  mSkinList = new SkinListWidget( base );
  mSkinList->setSortingEnabled( true );
  g->addWidget( mSkinList, 0, 0, 5, 1 );

  connect( mSkinList, SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ),
           SLOT( skinListCurrentItemChanged( QListWidgetItem *, QListWidgetItem * ) ) );

  mNewSkinButton = new QPushButton( i18n( "New Skin" ), base );
  mNewSkinButton->setIcon( KIcon( "document-new" ) );
  mNewSkinButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mNewSkinButton, 0, 1 );

  connect( mNewSkinButton, SIGNAL( clicked() ),
           SLOT( newSkinButtonClicked() ) );

  mCloneSkinButton = new QPushButton( i18n( "Clone Skin" ), base );
  mCloneSkinButton->setIcon( KIcon( "edit-copy" ) );
  mCloneSkinButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mCloneSkinButton, 1, 1 );

  connect( mCloneSkinButton, SIGNAL( clicked() ),
           SLOT( cloneSkinButtonClicked() ) );

  QFrame * f = new QFrame( base );
  f->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  f->setMinimumHeight( 24 );
  g->addWidget( f, 2, 1, Qt::AlignVCenter );

  mDeleteSkinButton = new QPushButton( i18n( "Delete Skin" ), base );
  mDeleteSkinButton->setIcon( KIcon( "edit-delete" ) );
  mDeleteSkinButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mDeleteSkinButton, 3, 1 );

  connect( mDeleteSkinButton, SIGNAL( clicked() ),
           SLOT( deleteSkinButtonClicked() ) );

  mEditor = new SkinEditor( base );
  g->addWidget( mEditor, 5, 0, 1, 2 );

  connect( mEditor, SIGNAL( skinNameChanged() ),
           SLOT( editedSkinNameChanged() ) );

  g->setColumnStretch( 0, 1 );
  g->setRowStretch( 4, 1 );

  connect( this, SIGNAL( okClicked() ),
           SLOT( okButtonClicked() ) );

  fillSkinList();
}

ConfigureSkinsDialog::~ConfigureSkinsDialog()
{
  mInstance = 0;
}

void ConfigureSkinsDialog::display( QWidget *parent, const QString &preselectSkinId )
{
  if ( mInstance )
    return; // FIXME: reparent if needed ?
  mInstance = new ConfigureSkinsDialog( parent );
  mInstance->selectSkinById( preselectSkinId );
  mInstance->show();
}

void ConfigureSkinsDialog::cleanup()
{
  if ( !mInstance )
    return;
  delete mInstance;
  mInstance = 0;
}

void ConfigureSkinsDialog::selectSkinById( const QString &skinId )
{
  SkinListWidgetItem * item = findSkinItemById( skinId );
  if ( !item )
    return;
  mSkinList->setCurrentItem( item );
}

void ConfigureSkinsDialog::okButtonClicked()
{
  commitEditor();

  Manager::instance()->removeAllSkins();

  int c = mSkinList->count();
  int i = 0;
  while ( i < c )
  {
    SkinListWidgetItem * item = dynamic_cast< SkinListWidgetItem * >( mSkinList->item( i ) );
    if ( item )
    {
      Manager::instance()->addSkin( item->skin() );
      item->forgetSkin();
    }
    i++;
  }

  Manager::instance()->skinsConfigurationCompleted();

  close(); // this will delete too
}

void ConfigureSkinsDialog::commitEditor()
{
  Skin * editedSkin = mEditor->editedSkin();
  if ( !editedSkin )
    return;

  mEditor->commit();

  SkinListWidgetItem * editedItem = findSkinItemBySkin( editedSkin );
  if ( editedItem )
    return;
  QString goodName = uniqueNameForSkin( editedSkin->name(), editedSkin );
  editedSkin->setName( goodName );
  editedItem->setText( goodName );
}

void ConfigureSkinsDialog::editedSkinNameChanged()
{
  Skin * set = mEditor->editedSkin();
  if ( !set )
    return;

  SkinListWidgetItem * it = findSkinItemBySkin( set );
  if ( !it )
    return;

  QString goodName = uniqueNameForSkin( set->name(), set );

  it->setText( goodName );
}

void ConfigureSkinsDialog::fillSkinList()
{
  const QHash< QString, Skin * > & sets = Manager::instance()->skins();
  for( QHash< QString, Skin * >::ConstIterator it = sets.begin(); it != sets.end(); ++it )
    (void)new SkinListWidgetItem( mSkinList, *( *it ) );
}

void ConfigureSkinsDialog::skinListCurrentItemChanged( QListWidgetItem * cur, QListWidgetItem * )
{
  commitEditor();

  SkinListWidgetItem * item = cur ? dynamic_cast< SkinListWidgetItem * >( cur ) : 0;
  mDeleteSkinButton->setEnabled( item && ( mSkinList->count() > 1 ) );
  mCloneSkinButton->setEnabled( item );
  mEditor->editSkin( item ? item->skin() : 0 );

  if ( item && !item->isSelected() )
    item->setSelected( true ); // make sure it's true
}

SkinListWidgetItem * ConfigureSkinsDialog::findSkinItemById( const QString &skinId )
{
  int c = mSkinList->count();
  int i = 0;
  while ( i < c )
  {
    SkinListWidgetItem * item = dynamic_cast< SkinListWidgetItem * >( mSkinList->item( i ) );
    if ( item )
    {
      if ( item->skin()->id() == skinId )
        return item;
    }
    i++;
  }
  return 0;
}


SkinListWidgetItem * ConfigureSkinsDialog::findSkinItemByName( const QString &name, Skin * skipSkin )
{
  int c = mSkinList->count();
  int i = 0;
  while ( i < c )
  {
    SkinListWidgetItem * item = dynamic_cast< SkinListWidgetItem * >( mSkinList->item( i ) );
    if ( item )
    {
      if ( item->skin() != skipSkin )
      {
        if ( item->skin()->name() == name )
          return item;
      }
    }
    i++;
  }
  return 0;
}

SkinListWidgetItem * ConfigureSkinsDialog::findSkinItemBySkin( Skin * set )
{
  int c = mSkinList->count();
  int i = 0;
  while ( i < c )
  {
    SkinListWidgetItem * item = dynamic_cast< SkinListWidgetItem * >( mSkinList->item( i ) );
    if ( item )
    {
      if ( item->skin() == set )
        return item;
    }
    i++;
  }
  return 0;
}


QString ConfigureSkinsDialog::uniqueNameForSkin( QString baseName, Skin * skipSkin )
{
  QString ret = baseName;
  if( ret.isEmpty() )
    ret = i18n( "Unnamed Skin" );

  int idx = 1;

  SkinListWidgetItem * item = findSkinItemByName( ret, skipSkin );
  while ( item )
  {
    idx++;
    ret = QString("%1 %2").arg( baseName ).arg( idx );
    item = findSkinItemByName( ret, skipSkin );
  }
  return ret;
}

void ConfigureSkinsDialog::newSkinButtonClicked()
{
  Skin emptySkin;
  emptySkin.setName( uniqueNameForSkin( i18n( "New Skin" ) ) );
  Skin::Column * col = new Skin::Column();
  col->setLabel( i18n( "New Column" ) );
  col->setVisibleByDefault( true );
  col->addMessageRow( new Skin::Row() );
  col->addGroupHeaderRow( new Skin::Row() );
  emptySkin.addColumn( col );
  SkinListWidgetItem * item = new SkinListWidgetItem( mSkinList, emptySkin );

  mSkinList->setCurrentItem( item );
}

void ConfigureSkinsDialog::cloneSkinButtonClicked()
{
  SkinListWidgetItem * item = dynamic_cast< SkinListWidgetItem * >( mSkinList->currentItem() );
  if ( !item )
    return;

  Skin copySkin( *( item->skin() ) );
  copySkin.generateUniqueId(); // regenerate id so it becomes different
  copySkin.setName( uniqueNameForSkin( item->skin()->name() ) );
  item = new SkinListWidgetItem( mSkinList, copySkin );

  mSkinList->setCurrentItem( item );

}

void ConfigureSkinsDialog::deleteSkinButtonClicked()
{
  SkinListWidgetItem * item = dynamic_cast< SkinListWidgetItem * >( mSkinList->currentItem() );
  if ( !item )
    return;
  if ( mSkinList->count() < 2 )
    return; // no way: desperately try to keep at least one option set alive :)

  mEditor->editSkin( 0 ); // forget it

  delete item; // this will trigger skinListCurrentItemChanged()
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

