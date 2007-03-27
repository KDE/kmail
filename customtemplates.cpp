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
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qtoolbox.h>
#include <kdebug.h>
#include <qfont.h>
#include <q3dict.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <k3listview.h>
#include <klineedit.h>
#include <qcombobox.h>
#include <kshortcut.h>
#include <kmessagebox.h>
#include <kkeybutton.h>

#include "ui_customtemplates_base.h"
#include "customtemplates_kfg.h"
#include "globalsettings.h"
#include "kmkernel.h"
#include "kmmainwidget.h"

#include "customtemplates.h"

CustomTemplates::CustomTemplates( QWidget *parent, const char *name )
  : QWidget(parent), mCurrentItem( 0 )
{
  setupUi(this);
  setObjectName(name);

  QFont f = KGlobalSettings::fixedFont();
  mEdit->setFont( f );

  mAdd->setIcon( KIcon( "list-add" ) );
  mRemove->setIcon( KIcon( "list-remove" ) );

  mList->setColumnWidth( 0, 50 );
  mList->setColumnWidth( 1, 100 );

  mEditFrame->setEnabled( false );

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

  connect( mKeyButton, SIGNAL( capturedShortcut( const KShortcut& ) ),
           this, SLOT( slotShortcutCaptured( const KShortcut& ) ) );

  mReplyPix = KIconLoader().loadIcon( "mail-reply-sender", K3Icon::Small );
  mReplyAllPix = KIconLoader().loadIcon( "mail-reply-all", K3Icon::Small );
  mForwardPix = KIconLoader().loadIcon( "mail-forward", K3Icon::Small );

  mType->clear();
  mType->insertItem( QPixmap(), i18nc( "Message->", "Universal" ), TUniversal );
  mType->insertItem( mReplyPix, i18nc( "Message->", "Reply" ), TReply );
  mType->insertItem( mReplyAllPix, i18nc( "Message->", "Reply to All" ), TReplyAll );
  mType->insertItem( mForwardPix, i18nc( "Message->", "Forward" ), TForward );

  QString help =
      i18n( "<qt>"
            "<p>Here you can add, edit, and delete custom message "
            "templates to use when you compose a reply or forwarding message. "
            "Create the custom template by selecting it using the right mouse "
            " button menu or toolbar menu. Also, you can bind a keyboard "
            "combination to the template for faster operations.</p>"
            "<p>Message templates support substitution commands, "
            "by simply typing them or selecting them from the "
            "<i>Insert command</i> menu.</p>"
            "<p>There are four types of custom templates: used to "
            "<i>Reply</i>, <i>Reply to All</i>, <i>Forward</i>, and "
            "<i>Universal</i> which can be used for all kinds of operations. "
            "You cannot bind a keyboard shortcut to <i>Universal</i> templates.</p>"
            "</qt>" );

  mHelp->setText( i18n( "<a href=\"whatsthis:%1\">How does this work?</a>", help ) );
  mHelp->setOpenExternalLinks(true);
  mHelp->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);
}

CustomTemplates::~CustomTemplates()
{
  Q3DictIterator<CustomTemplateItem> it(mItemList);
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
    // typeStr = i18n( "Any" ); break;
    break;
/*  case TNewMessage:
    typeStr = i18n( "New Message" ); break;*/
  case TReply:
    typeStr = i18nc( "Message->", "Reply" ); break;
  case TReplyAll:
    typeStr = i18nc( "Message->", "Reply to All" ); break;
  case TForward:
    typeStr = i18nc( "Message->", "Forward" ); break;
  default:
    typeStr = i18nc( "Message->", "Unknown" ); break;
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
    // QString typeStr = indexToType( t.type() );
    QString typeStr;
    KShortcut shortcut( t.shortcut() );
    CustomTemplateItem *vitem =
      new CustomTemplateItem( *it, t.content(),
        shortcut,
        static_cast<Type>( t.type() ) );
    mItemList.insert( *it, vitem );
    Q3ListViewItem *item = new Q3ListViewItem( mList, typeStr, *it, t.content() );
    switch ( t.type() ) {
    case TReply:
      item->setPixmap( 0, mReplyPix );
      break;
    case TReplyAll:
      item->setPixmap( 0, mReplyAllPix );
      break;
    case TForward:
      item->setPixmap( 0, mForwardPix );
      break;
    default:
      item->setPixmap( 0, QPixmap() );
      item->setText( 0, indexToType( t.type() ) );
      break;
    };
  }
}

void CustomTemplates::save()
{
  if ( mCurrentItem ) {
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      vitem->mContent = mEdit->text();
#ifdef __GNUC__
#warning Port me!
#endif
//      vitem->mShortcut = mKeyButton->shortcut();
    }
  }
  QStringList list;
  Q3ListViewItemIterator lit( mList );
  while ( lit.current() ) {
    list.append( (*lit)->text( 1 ) );
    ++lit;
  }
  Q3DictIterator<CustomTemplateItem> it( mItemList );
  for ( ; it.current() ; ++it ) {
    // list.append( (*it)->mName );
    CTemplates t( (*it)->mName );
    QString &content = (*it)->mContent;
    if ( content.trimmed().isEmpty() ) {
      content = "%BLANK";
    }
    t.setContent( content );
    t.setShortcut( (*it)->mShortcut.toString() );
    t.setType( (*it)->mType );
    t.writeConfig();
  }
  GlobalSettings::self()->setCustomTemplates( list );
  GlobalSettings::self()->writeConfig();

  // update kmail menus related to custom templates
  if ( kmkernel->getKMMainWidget() )
    kmkernel->getKMMainWidget()->updateCustomTemplateMenus();
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
      // KShortcut::null() doesn't seem to be present, although documented
      // at http://developer.kde.org/documentation/library/cvs-api/kdelibs-apidocs/kdecore/html/classKShortcut.html
#ifdef __GNUC__
#warning There must be a better way of doing this...
#endif
      KShortcut nullShortcut;
      vitem = new CustomTemplateItem( str, "", nullShortcut, TUniversal );
      mItemList.insert( str, vitem );
      Q3ListViewItem *item =
        new Q3ListViewItem( mList, indexToType( TUniversal ), str, "" );
      mList->setSelected( item, true );
      mKeyButton->setEnabled( false );
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
#ifdef __GNUC__
#warning Port me!
#endif
//      vitem->mShortcut = mKeyButton->shortcut();
    }
  }
  Q3ListViewItem *item = mList->selectedItem();
  if ( item ) {
    mEditFrame->setEnabled( true );
    mCurrentItem = item;
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      // avoid emit changed()
      disconnect( mEdit, SIGNAL( textChanged() ),
                  this, SLOT( slotTextChanged( void ) ) );

      mEdit->setText( vitem->mContent );
#ifdef __GNUC__
#warning Port me!
#endif
//      mKeyButton->setShortcut( vitem->mShortcut );
      mType->setCurrentItem( vitem->mType );

      connect( mEdit, SIGNAL( textChanged() ),
              this, SLOT( slotTextChanged( void ) ) );

      if ( vitem->mType == TUniversal )
      {
        mKeyButton->setEnabled( false );
      } else {
        mKeyButton->setEnabled( true );
      }
    }
  } else {
    mEditFrame->setEnabled( false );
    mCurrentItem = 0;
    mEdit->clear();
    // see above
#ifdef __GNUC__
#warning There must be a better way of doing this...
#endif
    KShortcut nullShortcut;
#ifdef __GNUC__
#warning Port me!
#endif
//    mKeyButton->setShortcut( nullShortcut );
    mType->setCurrentItem( 0 );
  }
}

void CustomTemplates::slotTypeActivated( int index )
{
  if ( mCurrentItem ) {
    // mCurrentItem->setText( 0, indexToType( index ) );
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( !vitem ) {
      return;
    }
    vitem->mType = static_cast<Type>(index);
    switch ( vitem->mType ) {
    case TReply:
      mCurrentItem->setPixmap( 0, mReplyPix );
      break;
    case TReplyAll:
      mCurrentItem->setPixmap( 0, mReplyAllPix );
      break;
    case TForward:
      mCurrentItem->setPixmap( 0, mForwardPix );
      break;
    default:
      mCurrentItem->setPixmap( 0, QPixmap() );
      break;
    };
    if ( index == TUniversal )
    {
      mKeyButton->setEnabled( false );
    } else {
      mKeyButton->setEnabled( true );
    }
    emit changed();
  }
}

void CustomTemplates::slotShortcutCaptured( const KShortcut &shortcut )
{
  KShortcut sc( shortcut );
#ifdef __GNUC__
#warning Port me!
#endif
//  if ( sc == mKeyButton->shortcut() )
    return;
  // see above
#ifdef __GNUC__
#warning There must be a better way of doing this...
#endif
  KShortcut nullShortcut;
  if ( sc == nullShortcut || sc.toString().isEmpty() )
    sc.clear();
  bool assign = true;
  bool customused = false;
  // check if shortcut is already used for custom templates
  Q3DictIterator<CustomTemplateItem> it(mItemList);
  for ( ; it.current() ; ++it ) {
    if ( !mCurrentItem || (*it)->mName != mCurrentItem->text( 1 ) )
    {
      if ( (*it)->mShortcut == sc )
      {
        QString title( I18N_NOOP("Key Conflict") );
        QString msg( I18N_NOOP("The selected shortcut is already used "
              "for another custom template, "
              "would you still like to continue with the assignment?" ) );
        assign = ( KMessageBox::warningYesNo( this, msg, title )
                    == KMessageBox::Yes );
        if ( assign )
        {
          (*it)->mShortcut.clear();
        }
        customused = true;
      }
    }
  }
  // check if shortcut is used somewhere else
  if ( !customused && sc != nullShortcut &&
       !( kmkernel->getKMMainWidget()->shortcutIsValid( sc ) ) ) {
    QString title( I18N_NOOP("Key Conflict") );
    QString msg( I18N_NOOP("The selected shortcut is already used, "
          "would you still like to continue with the assignment?" ) );
    assign = ( KMessageBox::warningYesNo( this, msg, title )
                == KMessageBox::Yes );
  }
  if ( assign ) {
#ifdef __GNUC__
#warning Port me!
#endif
//    mKeyButton->setShortcut( sc );
    emit changed();
  }
}

#include "customtemplates.moc"
