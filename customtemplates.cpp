/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>

#include <klocale.h>
#include <kglobal.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qtoolbox.h>
#include <kdebug.h>
#include <qfont.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <klistview.h>
#include <klineedit.h>
#include <qcombobox.h>

#include "customtemplates_base.h"
#include "customtemplates_kfg.h"
#include "globalsettings.h"

#include "customtemplates.h"

CustomTemplates::CustomTemplates( QWidget *parent, const char *name )
  :CustomTemplatesBase( parent, name ), mCurrentItem( 0 )
{
  QFont f = KGlobalSettings::fixedFont();
  mEdit->setFont( f );

  mAdd->setIconSet( BarIconSet( "add", KIcon::SizeSmall ) );
  mRemove->setIconSet( BarIconSet( "remove", KIcon::SizeSmall ) );

  mList->setColumnWidth( 0, 50 );
  mList->setColumnWidth( 1, 100 );

  connect( mEdit, SIGNAL( textChanged() ),
           this, SLOT( slotTextChanged( void ) ) );

  connect( mInsertCommand, SIGNAL( insertCommand(QString, int) ),
           this, SLOT( slotInsertCommand(QString, int) ) );

  connect( mAdd, SIGNAL( clicked() ),
           this, SLOT( slotAddClicked() ) );
  connect( mRemove, SIGNAL( clicked() ),
           this, SLOT( slotRemoveClicked() ) );
  connect( mList, SIGNAL( selectionChanged() ),
           this, SLOT( slotListSelectionChanged() ) );
  connect( mType, SIGNAL( activated( int ) ),
           this, SLOT( slotTypeActivated( int ) ) );

}

CustomTemplates::~CustomTemplates()
{
  QDictIterator<CustomTemplateItem> it(mItemList);
  for ( ; it.current() ; ++it ) {
    CustomTemplateItem *vitem = mItemList.take( it.currentKey() );
    if ( vitem ) {
      delete vitem;
    }
  }
}

QString CustomTemplates::indexToType( int index )
{
  QString typeStr;
  switch ( index ) {
  case TUniversal:
    typeStr = i18n( "Any" ); break;
/*  case TNewMessage:
    typeStr = i18n( "New Message" ); break;*/
  case TReply:
    typeStr = i18n( "Reply" ); break;
  case TReplyAll:
    typeStr = i18n( "Reply to All" ); break;
  case TForward:
    typeStr = i18n( "Forward" ); break;
  default:
    typeStr = i18n( "Unknown" ); break;
  }
  return typeStr;
}

void CustomTemplates::slotTextChanged()
{
  emit changed();
}

void CustomTemplates::load()
{
  QStringList list = GlobalSettings::self()->customTemplates();
  for ( QStringList::iterator it = list.begin(); it != list.end(); ++it ) {
    CTemplates t(*it);
    QString typeStr = indexToType( t.type() );
    CustomTemplateItem *vitem =
      new CustomTemplateItem( *it, t.content(), static_cast<Type>( t.type() ) );
    mItemList.insert( *it, vitem );
    (void) new QListViewItem( mList, typeStr, *it, t.content() );
  }
}

void CustomTemplates::save()
{
  if ( mCurrentItem ) {
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      vitem->mContent = mEdit->text();
    }
  }
  QStringList list;
  QDictIterator<CustomTemplateItem> it(mItemList);
  for ( ; it.current() ; ++it ) {
    list.append( (*it)->mName );
    CTemplates t( (*it)->mName );
    t.setContent( (*it)->mContent );
    t.setType( (*it)->mType );
    t.writeConfig();
  }
  GlobalSettings::self()->setCustomTemplates( list );
  GlobalSettings::self()->writeConfig();
}

void CustomTemplates::slotInsertCommand( QString cmd, int adjustCursor )
{
  int para, index;
  mEdit->getCursorPosition( &para, &index );
  mEdit->insertAt( cmd, para, index );

  index += adjustCursor;

  mEdit->setCursorPosition( para, index + cmd.length() );
}

void CustomTemplates::slotAddClicked()
{
  QString str = mName->text();
  if ( !str.isEmpty() ) {
    CustomTemplateItem *vitem = mItemList[ str ];
    if ( !vitem ) {
      vitem = new CustomTemplateItem( str, "", TUniversal );
      mItemList.insert( str, vitem );
      QListViewItem *item =
        new QListViewItem( mList, indexToType( TUniversal ), str, "" );
      mList->setSelected( item, true );
      emit changed();
    }
  }
}

void CustomTemplates::slotRemoveClicked()
{
  if ( mCurrentItem ) {
    CustomTemplateItem *vitem = mItemList.take( mCurrentItem->text( 1 ) );
    if ( vitem ) {
      delete vitem;
    }
    delete mCurrentItem;
    mCurrentItem = 0;
    emit changed();
  }
}

void CustomTemplates::slotListSelectionChanged()
{
  if ( mCurrentItem ) {
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      vitem->mContent = mEdit->text();
    }
  }
  QListViewItem *item = mList->selectedItem();
  if ( item ) {
    mCurrentItem = item;
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      // avoid emit changed()
      disconnect( mEdit, SIGNAL( textChanged() ),
                  this, SLOT( slotTextChanged( void ) ) );

      mEdit->setText( vitem->mContent );
      mType->setCurrentItem( vitem->mType );

      connect( mEdit, SIGNAL( textChanged() ),
              this, SLOT( slotTextChanged( void ) ) );
    }
  } else {
    mCurrentItem = 0;
    mEdit->clear();
    mType->setCurrentItem( 0 );
  }
}

void CustomTemplates::slotTypeActivated( int index )
{
  if ( mCurrentItem ) {
    mCurrentItem->setText( 0, indexToType( index ) );
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      vitem->mType = static_cast<Type>(index);
    }
    emit changed();
  }
}

#include "customtemplates.moc"
