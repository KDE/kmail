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

#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtoolbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qfont.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <klistview.h>
#include <klineedit.h>
#include <qcombobox.h>
#include <kshortcut.h>
#include <kmessagebox.h>
#include <kkeybutton.h>
#include <kactivelabel.h>

#include "customtemplates_base.h"
#include "customtemplates_kfg.h"
#include "globalsettings.h"
#include "kmkernel.h"
#include "kmmainwidget.h"
#include "kmfawidgets.h"

#include "customtemplates.h"

CustomTemplates::CustomTemplates( QWidget *parent, const char *name )
  :CustomTemplatesBase( parent, name ),
   mCurrentItem( 0 ),
   mBlockChangeSignal( false )
{
  QFont f = KGlobalSettings::fixedFont();
  mEdit->setFont( f );

  mAdd->setIconSet( BarIconSet( "add", KIcon::SizeSmall ) );
  mRemove->setIconSet( BarIconSet( "remove", KIcon::SizeSmall ) );

  mList->setColumnWidth( 0, 50 );
  mList->setColumnWidth( 1, 100 );

  mEditFrame->setEnabled( false );

  connect( mName, SIGNAL( textChanged ( const QString &) ),
           this, SLOT( slotNameChanged( const QString & ) ) );
  connect( mEdit, SIGNAL( textChanged() ),
           this, SLOT( slotTextChanged( void ) ) );
  connect( mToEdit, SIGNAL( textChanged(const QString&) ),
           this, SLOT( slotTextChanged( void ) ) );
  connect( mCCEdit, SIGNAL( textChanged(const QString&) ),
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

  mReplyPix = KIconLoader().loadIcon( "mail_reply", KIcon::Small );
  mReplyAllPix = KIconLoader().loadIcon( "mail_replyall", KIcon::Small );
  mForwardPix = KIconLoader().loadIcon( "mail_forward", KIcon::Small );

  mType->clear();
  mType->insertItem( QPixmap(), i18n( "Message->", "Universal" ), TUniversal );
  mType->insertItem( mReplyPix, i18n( "Message->", "Reply" ), TReply );
  mType->insertItem( mReplyAllPix, i18n( "Message->", "Reply to All" ), TReplyAll );
  mType->insertItem( mForwardPix, i18n( "Message->", "Forward" ), TForward );

  QString help =
      i18n( "<qt>"
            "<p>Here you can add, edit, and delete custom message "
            "templates to use when you compose a reply or forwarding message. "
            "Create the custom template by selecting it using the right mouse "
            " button menu or toolbar menu. Also, you can bind a keyboard "
            "combination to the template for faster operations.</p>"
            "<p>Message templates support substitution commands "
            "by simple typing them or selecting them from menu "
            "<i>Insert command</i>.</p>"
            "<p>There are four types of custom templates: used to "
            "<i>Reply</i>, <i>Reply to All</i>, <i>Forward</i>, and "
            "<i>Universal</i> which can be used for all kind of operations. "
            "You cannot bind keyboard shortcut to <i>Universal</i> templates.</p>"
            "</qt>" );
  mHelp->setText( i18n( "<a href=\"whatsthis:%1\">How does this work?</a>" ).arg( help ) );

  const QString toToolTip = i18n( "Additional recipients of the message when forwarding" );
  const QString ccToolTip = i18n( "Additional recipients who get a copy of the message when forwarding" );
  const QString toWhatsThis = i18n( "When using this template for forwarding, the default recipients are those you enter here. This is a comma-separated list of mail addresses." );
  const QString ccWhatsThis = i18n( "When using this template for forwarding, the recipients you enter here will by default get a copy of this message. This is a comma-separated list of mail addresses." );

  // We only want to set the tooltip/whatsthis to the lineedit, not the complete widget,
  // so we use the name here to find the lineedit. This is similar to what KMFilterActionForward
  // does.
  KLineEdit *ccLineEdit = dynamic_cast<KLineEdit*>( mCCEdit->child( "addressEdit" ) );
  KLineEdit *toLineEdit = dynamic_cast<KLineEdit*>( mToEdit->child( "addressEdit" ) );
  Q_ASSERT( ccLineEdit && toLineEdit );

  QToolTip::add( mCCLabel, ccToolTip );
  QToolTip::add( ccLineEdit, ccToolTip );
  QToolTip::add( mToLabel, toToolTip );
  QToolTip::add( toLineEdit, toToolTip );
  QWhatsThis::add( mCCLabel, ccWhatsThis );
  QWhatsThis::add( ccLineEdit, ccWhatsThis );
  QWhatsThis::add( mToLabel, toWhatsThis );
  QWhatsThis::add( toLineEdit, toWhatsThis );

  slotNameChanged( mName->text() );
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

void CustomTemplates::setRecipientsEditsEnabled( bool enabled )
{
  mToEdit->setHidden( !enabled );
  mCCEdit->setHidden( !enabled );
  mToLabel->setHidden( !enabled );
  mCCLabel->setHidden( !enabled );
}

void CustomTemplates::slotNameChanged( const QString& text )
{
  mAdd->setEnabled( !text.isEmpty() );
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
    typeStr = i18n( "Message->", "Reply" ); break;
  case TReplyAll:
    typeStr = i18n( "Message->", "Reply to All" ); break;
  case TForward:
    typeStr = i18n( "Message->", "Forward" ); break;
  default:
    typeStr = i18n( "Message->", "Unknown" ); break;
  }
  return typeStr;
}

void CustomTemplates::slotTextChanged()
{
  if ( !mBlockChangeSignal )
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
        static_cast<Type>( t.type() ), t.to(), t.cC() );
    mItemList.insert( *it, vitem );
    QListViewItem *item = new QListViewItem( mList, typeStr, *it, t.content() );
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
  // Before saving the new templates, delete the old ones. That needs to be done before
  // saving, since otherwise a new template with the new name wouldn't get saved.
  for ( QStringList::const_iterator it = mItemsToDelete.constBegin();
        it != mItemsToDelete.constEnd(); ++it ) {
    CTemplates t( (*it) );
    const QString configGroup = t.currentGroup();
    kmkernel->config()->deleteGroup( configGroup );
  }

  if ( mCurrentItem ) {
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      vitem->mContent = mEdit->text();
      vitem->mShortcut = mKeyButton->shortcut();
      vitem->mTo = mToEdit->text();
      vitem->mCC = mCCEdit->text();
    }
  }
  QStringList list;
  QListViewItemIterator lit( mList );
  while ( lit.current() ) {
    list.append( (*lit)->text( 1 ) );
    ++lit;
  }
  for ( QDictIterator<CustomTemplateItem> it( mItemList ); it.current() ; ++it ) {
    // list.append( (*it)->mName );
    CTemplates t( (*it)->mName );
    QString &content = (*it)->mContent;
    if ( content.stripWhiteSpace().isEmpty() ) {
      content = "%BLANK";
    }
    t.setContent( content );
    t.setShortcut( (*it)->mShortcut.toString() );
    t.setType( (*it)->mType );
    t.setTo( (*it)->mTo );
    t.setCC( (*it)->mCC );
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
      vitem = new CustomTemplateItem( str, "", KShortcut::null(), TUniversal,
                                      QString(), QString() );
      mItemList.insert( str, vitem );
      QListViewItem *item =
        new QListViewItem( mList, indexToType( TUniversal ), str, "" );
      mList->setSelected( item, true );
      mKeyButton->setEnabled( false );
      if ( !mBlockChangeSignal )
        emit changed();
    }
  }
}

void CustomTemplates::slotRemoveClicked()
{
  if ( mCurrentItem ) {
    const QString templateName = mCurrentItem->text( 1 );
    mItemsToDelete.append( templateName );
    CustomTemplateItem *vitem = mItemList.take( templateName );
    delete vitem;
    delete mCurrentItem;
    mCurrentItem = 0;
    if ( !mBlockChangeSignal )
      emit changed();
  }
}

void CustomTemplates::slotListSelectionChanged()
{
  if ( mCurrentItem ) {
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {
      vitem->mContent = mEdit->text();
      vitem->mShortcut = mKeyButton->shortcut();
    }
  }
  QListViewItem *item = mList->selectedItem();
  if ( item ) {
    mEditFrame->setEnabled( true );
    mCurrentItem = item;
    CustomTemplateItem *vitem = mItemList[ mCurrentItem->text( 1 ) ];
    if ( vitem ) {

      mBlockChangeSignal = true;
      mEdit->setText( vitem->mContent );
      mKeyButton->setShortcut( vitem->mShortcut, false );
      mType->setCurrentItem( vitem->mType );
      mToEdit->setText( vitem->mTo );
      mCCEdit->setText( vitem->mCC );
      mBlockChangeSignal = false;

      if ( vitem->mType == TUniversal )
      {
        mKeyButton->setEnabled( false );
      } else {
        mKeyButton->setEnabled( true );
      }
      setRecipientsEditsEnabled( vitem->mType == TForward ||
                                 vitem->mType == TUniversal );
    }
  } else {
    mEditFrame->setEnabled( false );
    mCurrentItem = 0;
    mEdit->clear();
    mToEdit->clear();
    mCCEdit->clear();
    mKeyButton->setShortcut( KShortcut::null(), false );
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

    setRecipientsEditsEnabled( vitem->mType == TForward ||
                               vitem->mType == TUniversal );
    if ( !mBlockChangeSignal )
      emit changed();
  }
  else
    setRecipientsEditsEnabled( false );
}

void CustomTemplates::slotShortcutCaptured( const KShortcut &shortcut )
{
  KShortcut sc( shortcut );
  if ( sc == mKeyButton->shortcut() )
    return;
  if ( sc.isNull() || sc.toString().isEmpty() )
    sc.clear();
  bool assign = true;
  bool customused = false;
  // check if shortcut is already used for custom templates
  QDictIterator<CustomTemplateItem> it(mItemList);
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
          (*it)->mShortcut = KShortcut::null();
        }
        customused = true;
      }
    }
  }
  // check if shortcut is used somewhere else
  if ( !customused && !sc.isNull() &&
       !( kmkernel->getKMMainWidget()->shortcutIsValid( sc ) ) ) {
    QString title( I18N_NOOP("Key Conflict") );
    QString msg( I18N_NOOP("The selected shortcut is already used, "
          "would you still like to continue with the assignment?" ) );
    assign = ( KMessageBox::warningYesNo( this, msg, title )
                == KMessageBox::Yes );
  }
  if ( assign ) {
    mKeyButton->setShortcut( sc, false );
    if ( !mBlockChangeSignal )
      emit changed();
  }
}

#include "customtemplates.moc"
