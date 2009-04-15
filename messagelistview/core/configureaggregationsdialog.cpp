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

#include "messagelistview/core/configureaggregationsdialog.h"

#include "messagelistview/core/aggregationeditor.h"
#include "messagelistview/core/aggregation.h"

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

class AggregationListWidgetItem : public QListWidgetItem
{
private:
  Aggregation * mAggregation;

public:
  AggregationListWidgetItem( QListWidget * par, const Aggregation &set )
    : QListWidgetItem( set.name(), par )
  {
    mAggregation = new Aggregation( set );
  }
  ~AggregationListWidgetItem()
  {
    delete mAggregation;
  }

public:
  Aggregation * aggregation() const
    { return mAggregation; };
  void forgetAggregation()
    { mAggregation = 0; };
};

ConfigureAggregationsDialog * ConfigureAggregationsDialog::mInstance = 0;

ConfigureAggregationsDialog::ConfigureAggregationsDialog( QWidget *parent )
  : KDialog( parent )
{
  mInstance = this;
  setAttribute( Qt::WA_DeleteOnClose );
  setWindowModality( Qt::ApplicationModal ); // FIXME: Sure ?
  setButtons( Ok | Cancel );
  setWindowTitle( i18n( "Customize Message Aggregation Modes" ) );

  QWidget * base = new QWidget( this );
  setMainWidget( base );

  QGridLayout * g = new QGridLayout( base );

  mAggregationList = new AggregationListWidget( base );
  mAggregationList->setSortingEnabled( true );
  g->addWidget( mAggregationList, 0, 0, 5, 1 );

  connect( mAggregationList, SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ),
           SLOT( aggregationListCurrentItemChanged( QListWidgetItem *, QListWidgetItem * ) ) );

  mNewAggregationButton = new QPushButton( i18n( "New Aggregation" ), base );
  mNewAggregationButton->setIcon( KIcon( "document-new" ) );
  mNewAggregationButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mNewAggregationButton, 0, 1 );

  connect( mNewAggregationButton, SIGNAL( clicked() ),
           SLOT( newAggregationButtonClicked() ) );

  mCloneAggregationButton = new QPushButton( i18n( "Clone Aggregation" ), base );
  mCloneAggregationButton->setIcon( KIcon( "edit-copy" ) );
  mCloneAggregationButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mCloneAggregationButton, 1, 1 );

  connect( mCloneAggregationButton, SIGNAL( clicked() ),
           SLOT( cloneAggregationButtonClicked() ) );

  QFrame * f = new QFrame( base );
  f->setFrameStyle( QFrame::Sunken | QFrame::HLine );
  f->setMinimumHeight( 24 );
  g->addWidget( f, 2, 1, Qt::AlignVCenter );

  mDeleteAggregationButton = new QPushButton( i18n( "Delete Aggregation" ), base );
  mDeleteAggregationButton->setIcon( KIcon( "edit-delete" ) );
  mDeleteAggregationButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  g->addWidget( mDeleteAggregationButton, 3, 1 );

  connect( mDeleteAggregationButton, SIGNAL( clicked() ),
           SLOT( deleteAggregationButtonClicked() ) );

  mEditor = new AggregationEditor( base );
  g->addWidget( mEditor, 5, 0, 1, 2 );

  connect( mEditor, SIGNAL( aggregationNameChanged() ),
           SLOT( editedAggregationNameChanged() ) );

  g->setColumnStretch( 0, 1 );
  g->setRowStretch( 4, 1 );

  connect( this, SIGNAL( okClicked() ),
           SLOT( okButtonClicked() ) );

  fillAggregationList();
}

ConfigureAggregationsDialog::~ConfigureAggregationsDialog()
{
  mInstance = 0;
}

void ConfigureAggregationsDialog::display( QWidget *parent, const QString &preselectAggregationId )
{
  if ( mInstance )
    return; // FIXME: reparent if needed ?
  mInstance = new ConfigureAggregationsDialog( parent );
  mInstance->selectAggregationById( preselectAggregationId );
  mInstance->show();
}

void ConfigureAggregationsDialog::cleanup()
{
  if ( !mInstance )
    return;
  delete mInstance;
  mInstance = 0;
}

void ConfigureAggregationsDialog::selectAggregationById( const QString &aggregationId )
{
  AggregationListWidgetItem * item = findAggregationItemById( aggregationId );
  if ( !item )
    return;
  mAggregationList->setCurrentItem( item );
}

void ConfigureAggregationsDialog::okButtonClicked()
{
  commitEditor();

  Manager::instance()->removeAllAggregations();

  int c = mAggregationList->count();
  int i = 0;
  while ( i < c )
  {
    AggregationListWidgetItem * item = dynamic_cast< AggregationListWidgetItem * >( mAggregationList->item( i ) );
    if ( item )
    {
      Manager::instance()->addAggregation( item->aggregation() );
      item->forgetAggregation();
    }
    i++;
  }

  Manager::instance()->aggregationsConfigurationCompleted();

  close(); // this will delete too
}

void ConfigureAggregationsDialog::commitEditor()
{
  Aggregation * editedAggregation = mEditor->editedAggregation();
  if ( !editedAggregation )
    return;

  mEditor->commit();

  AggregationListWidgetItem * editedItem = findAggregationItemByAggregation( editedAggregation );
  if ( editedItem )
    return;
  QString goodName = uniqueNameForAggregation( editedAggregation->name(), editedAggregation );
  editedAggregation->setName( goodName );
  editedItem->setText( goodName );
}

void ConfigureAggregationsDialog::editedAggregationNameChanged()
{
  Aggregation * set = mEditor->editedAggregation();
  if ( !set )
    return;

  AggregationListWidgetItem * it = findAggregationItemByAggregation( set );
  if ( !it )
    return;

  QString goodName = uniqueNameForAggregation( set->name(), set );

  it->setText( goodName );
}

void ConfigureAggregationsDialog::fillAggregationList()
{
  const QHash< QString, Aggregation * > & sets = Manager::instance()->aggregations();
  for( QHash< QString, Aggregation * >::ConstIterator it = sets.begin(); it != sets.end(); ++it )
    (void)new AggregationListWidgetItem( mAggregationList, *( *it ) );
}

void ConfigureAggregationsDialog::aggregationListCurrentItemChanged( QListWidgetItem * cur, QListWidgetItem * )
{
  commitEditor();

  AggregationListWidgetItem * item = cur ? dynamic_cast< AggregationListWidgetItem * >( cur ) : 0;
  mDeleteAggregationButton->setEnabled( item && ( mAggregationList->count() > 1 ) );
  mCloneAggregationButton->setEnabled( item );
  mEditor->editAggregation( item ? item->aggregation() : 0 );
  if ( item && !item->isSelected() )
    item->setSelected( true ); // make sure it's true
}

AggregationListWidgetItem * ConfigureAggregationsDialog::findAggregationItemByName( const QString &name, Aggregation * skipAggregation )
{
  int c = mAggregationList->count();
  int i = 0;
  while ( i < c )
  {
    AggregationListWidgetItem * item = dynamic_cast< AggregationListWidgetItem * >( mAggregationList->item( i ) );
    if ( item )
    {
      if ( item->aggregation() != skipAggregation )
      {
        if ( item->aggregation()->name() == name )
          return item;
      }
    }
    i++;
  }
  return 0;
}

AggregationListWidgetItem * ConfigureAggregationsDialog::findAggregationItemById( const QString &aggregationId )
{
  int c = mAggregationList->count();
  int i = 0;
  while ( i < c )
  {
    AggregationListWidgetItem * item = dynamic_cast< AggregationListWidgetItem * >( mAggregationList->item( i ) );
    if ( item )
    {
      if ( item->aggregation()->id() == aggregationId )
        return item;
    }
    i++;
  }
  return 0;
}

AggregationListWidgetItem * ConfigureAggregationsDialog::findAggregationItemByAggregation( Aggregation * set )
{
  int c = mAggregationList->count();
  int i = 0;
  while ( i < c )
  {
    AggregationListWidgetItem * item = dynamic_cast< AggregationListWidgetItem * >( mAggregationList->item( i ) );
    if ( item )
    {
      if ( item->aggregation() == set )
        return item;
    }
    i++;
  }
  return 0;
}


QString ConfigureAggregationsDialog::uniqueNameForAggregation( QString baseName, Aggregation * skipAggregation )
{
  QString ret = baseName;
  if( ret.isEmpty() )
    ret = i18n( "Unnamed Aggregation" );

  int idx = 1;

  AggregationListWidgetItem * item = findAggregationItemByName( ret, skipAggregation );
  while ( item )
  {
    idx++;
    ret = QString("%1 %2").arg( baseName ).arg( idx );
    item = findAggregationItemByName( ret, skipAggregation );
  }
  return ret;
}

void ConfigureAggregationsDialog::newAggregationButtonClicked()
{
  Aggregation emptyAggregation;
  emptyAggregation.setName( uniqueNameForAggregation( i18n( "New Aggregation" ) ) );
  AggregationListWidgetItem * item = new AggregationListWidgetItem( mAggregationList, emptyAggregation );

  mAggregationList->setCurrentItem( item );
}

void ConfigureAggregationsDialog::cloneAggregationButtonClicked()
{
  AggregationListWidgetItem * item = dynamic_cast< AggregationListWidgetItem * >( mAggregationList->currentItem() );
  if ( !item )
    return;

  Aggregation copyAggregation( *( item->aggregation() ) );
  copyAggregation.generateUniqueId(); // regenerate id so it becomes different
  copyAggregation.setName( uniqueNameForAggregation( item->aggregation()->name() ) );
  item = new AggregationListWidgetItem( mAggregationList, copyAggregation );

  mAggregationList->setCurrentItem( item );

}

void ConfigureAggregationsDialog::deleteAggregationButtonClicked()
{
  AggregationListWidgetItem * item = dynamic_cast< AggregationListWidgetItem * >( mAggregationList->currentItem() );
  if ( !item )
    return;
  if ( mAggregationList->count() < 2 )
    return; // no way: desperately try to keep at least one option set alive :)

  mEditor->editAggregation( 0 ); // forget it

  delete item; // this will trigger aggregationListCurrentItemChanged()
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

