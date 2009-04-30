/*   -*- mode: C++; c-file-style: "gnu" -*-
*   kmail: KDE mail client
*   Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
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
*   You should have received a copy of the GNU General Public License along
*   with this program; if not, write to the Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "customtemplates.h"

#include <qfont.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <QWhatsThis>
#include <qtoolbox.h>
#include <QHeaderView>

#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <kshortcut.h>
#include <kmessagebox.h>
#include <kkeysequencewidget.h>

#include "ui_customtemplates_base.h"
#include "customtemplates_kfg.h"
#include "globalsettings.h"
#include "kmkernel.h"
#include "kmmainwidget.h"

CustomTemplates::CustomTemplates( QWidget *parent, const char *name )
  : QWidget(parent)
{
  setupUi(this);
  setObjectName(name);

  QFont f = KGlobalSettings::fixedFont();
  mEdit->setFont( f );

  mAdd->setIcon( KIcon( "list-add" ) );
  mAdd->setEnabled( false );
  mRemove->setIcon( KIcon( "list-remove" ) );

  mList->setColumnWidth( 0, 100 );
  mList->header()->setStretchLastSection( true );

  mEditFrame->setEnabled( false );

  connect( mEdit, SIGNAL( textChanged() ),
          this, SLOT( slotTextChanged( void ) ) );

  connect( mName, SIGNAL( textChanged( const QString & ) ),
           this, SLOT( slotNameChanged( const QString & ) ) );

  connect( mInsertCommand, SIGNAL( insertCommand(const QString&, int) ),
          this, SLOT( slotInsertCommand(const QString&, int) ) );

  connect( mAdd, SIGNAL( clicked() ),
          this, SLOT( slotAddClicked() ) );
  connect( mRemove, SIGNAL( clicked() ),
          this, SLOT( slotRemoveClicked() ) );
  connect( mList, SIGNAL( itemSelectionChanged() ),
          this, SLOT( slotListSelectionChanged() ) );
  connect( mType, SIGNAL( activated( int ) ),
          this, SLOT( slotTypeActivated( int ) ) );

  connect( mKeySequenceWidget, SIGNAL( keySequenceChanged( const QKeySequence& ) ),
          this, SLOT( slotShortcutChanged( const QKeySequence& ) ) );

  mKeySequenceWidget->setCheckActionCollections(
      kmkernel->getKMMainWidget()->actionCollections() );

  mReplyPix = KIconLoader().loadIcon( "mail-reply-sender", KIconLoader::Small );
  mReplyAllPix = KIconLoader().loadIcon( "mail-reply-all", KIconLoader::Small );
  mForwardPix = KIconLoader().loadIcon( "mail-forward", KIconLoader::Small );

  mType->clear();
  mType->addItem( QPixmap(), i18nc( "Message->", "Universal" ) );
  mType->addItem( mReplyPix, i18nc( "Message->", "Reply" ) );
  mType->addItem( mReplyAllPix, i18nc( "Message->", "Reply to All" ) );
  mType->addItem( mForwardPix, i18nc( "Message->", "Forward" ) );

  mHelp->setText( i18n( "<a href=\"whatsthis\">How does this work?</a>" ) );
  connect( mHelp, SIGNAL( linkActivated ( const QString& ) ),
          SLOT( slotHelpLinkClicked( const QString& ) ) );
}

void CustomTemplates::slotHelpLinkClicked( const QString& )
{
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

  QWhatsThis::showText( QCursor::pos(), help );
}

CustomTemplates::~CustomTemplates()
{
  qDeleteAll( mItemList ); // no auto-delete with QHash
}

QString CustomTemplates::indexToType( int index )
{
  QString typeStr;
  switch ( index ) {
  case TUniversal:
    typeStr = i18nc( "Message->", "Universal" ); break;
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

void CustomTemplates::slotNameChanged( const QString & text )
{
  mAdd->setEnabled( !text.isEmpty() );
}

void CustomTemplates::slotTextChanged()
{
  if ( mList->currentItem() ) {
    CustomTemplateItem *vitem = mItemList[ mList->currentItem()->text( 1 ) ];
    if ( vitem )
      vitem->mContent = mEdit->toPlainText();
  }

  emit changed();
}

void CustomTemplates::load()
{
  QStringList list = GlobalSettings::self()->customTemplates();
  for ( QStringList::const_iterator it = list.constBegin(); it != list.constEnd(); ++it ) {
    CTemplates t(*it);
    // QString typeStr = indexToType( t.type() );
    QString typeStr;
    KShortcut shortcut( t.shortcut() );
    CustomTemplateItem *vitem =
      new CustomTemplateItem( *it, t.content(),
        shortcut,
        static_cast<Type>( t.type() ) );
    mItemList.insert( *it, vitem );
    QTreeWidgetItem *item = new QTreeWidgetItem( mList );
    item->setText( 0, typeStr );
    item->setText( 1, *it );
    item->setText( 0, indexToType( t.type() ) );
    switch ( t.type() ) {
    case TReply:
      item->setIcon( 0, mReplyPix );
      break;
    case TReplyAll:
      item->setIcon( 0, mReplyAllPix );
      break;
    case TForward:
      item->setIcon( 0, mForwardPix );
      break;
    default:
      item->setIcon( 0, QPixmap() );
      break;
    };
  }

  mRemove->setEnabled( mList->topLevelItemCount() > 0 );
}

void CustomTemplates::save()
{
  // Before saving the new templates, delete the old ones. That needs to be done before
  // saving, since otherwise a new template with the new name wouldn't get saved.
  foreach( const QString &item, mItemsToDelete ) {
    CTemplates t( item );
    const QString configGroup = t.currentGroup();
    kmkernel->config()->deleteGroup( configGroup );
  }

  QStringList list;
  QTreeWidgetItemIterator lit( mList );
  while ( *lit ) {
    list.append( (*lit)->text( 1 ) );
    ++lit;
  }

  foreach( const CustomTemplateItem *item, mItemList ) {
    const CustomTemplateItem *ti = item;
    CTemplates t( ti->mName );
    QString content = ti->mContent;
    if ( content.trimmed().isEmpty() ) {
      content = "%BLANK";
    }
    t.setContent( content );
    t.setShortcut( ti->mShortcut.toString() );
    t.setType( ti->mType );
    t.writeConfig();
  }
  GlobalSettings::self()->setCustomTemplates( list );
  GlobalSettings::self()->writeConfig();

  // update all menus related to custom templates
  kmkernel->updatedTemplates();
}

void CustomTemplates::slotInsertCommand( const QString &cmd, int adjustCursor )
{
  QTextCursor cursor = mEdit->textCursor();
  cursor.insertText( cmd );
  cursor.setPosition( cursor.position() + adjustCursor );
  mEdit->setTextCursor( cursor );
  mEdit->setFocus();
}

void CustomTemplates::slotAddClicked()
{
  QString str = mName->text();
  if ( !str.isEmpty() ) {
    CustomTemplateItem *vitem = mItemList[ str ];
    if ( !vitem ) {
      // KShortcut::null() doesn't seem to be present, although documented
      // at http://developer.kde.org/documentation/library/cvs-api/kdelibs-apidocs/kdecore/html/classKShortcut.html
      // see slotShortcutChanged(). oh, and you should look up documentation on the english breakfast network!
      // FIXME There must be a better way of doing this...
      KShortcut nullShortcut;
      vitem = new CustomTemplateItem( str, "", nullShortcut, TUniversal );
      mItemList.insert( str, vitem );
      QTreeWidgetItem *item =
        new QTreeWidgetItem( mList );
      item->setText( 0, indexToType( TUniversal ) );
      item->setText( 1, str );
      mList->setCurrentItem( item );
      mRemove->setEnabled( true );
      mName->clear();
      mKeySequenceWidget->setEnabled( false );
      emit changed();
    }
  }
}

void CustomTemplates::slotRemoveClicked()
{
  if ( !mList->currentItem() )
    return;

  const QString templateName = mList->currentItem()->text( 1 );
  mItemsToDelete.append( templateName );
  delete mItemList.take( templateName );
  delete mList->takeTopLevelItem( mList->indexOfTopLevelItem (
                                  mList->currentItem() ) );
  mRemove->setEnabled( mList->topLevelItemCount() > 0 );
  emit changed();
}

void CustomTemplates::slotListSelectionChanged()
{
  QTreeWidgetItem *item = mList->currentItem();
  if ( item ) {
    mEditFrame->setEnabled( true );
    CustomTemplateItem *vitem = mItemList[ mList->currentItem()->text( 1 ) ];
    if ( vitem ) {
      // avoid emit changed()
      disconnect( mEdit, SIGNAL( textChanged() ),
                  this, SLOT( slotTextChanged( void ) ) );

      mEdit->setText( vitem->mContent );
      mKeySequenceWidget->setKeySequence( vitem->mShortcut.primary(),
                                          KKeySequenceWidget::NoValidate );
      mType->setCurrentIndex( mType->findText( indexToType ( vitem->mType ) ) );

      connect( mEdit, SIGNAL( textChanged() ),
              this, SLOT( slotTextChanged( void ) ) );

      // I think the logic (originally 'vitem->mType==TUniversal') was inverted here:
      // a key shortcut is only allowed for a specific type of template and not for
      // a universal, as otherwise we won't know what sort of action to do when the
      // key sequence is activated!
      // This agrees with KMMainWidget::updateCustomTemplateMenus() -- marten
      mKeySequenceWidget->setEnabled( vitem->mType != TUniversal );
    }
  } else {
    mEditFrame->setEnabled( false );
    mEdit->clear();
    // see above
    mKeySequenceWidget->clearKeySequence();
    mType->setCurrentIndex( 0 );
  }
}

void CustomTemplates::slotTypeActivated( int index )
{
  if ( mList->currentItem() ) {
    // mCurrentItem->setText( 0, indexToType( index ) );
    CustomTemplateItem *vitem = mItemList[ mList->currentItem()->text( 1 ) ];
    if ( !vitem )
      return;

    vitem->mType = static_cast<Type>(index);
    mList->currentItem()->setText( 0, indexToType( vitem->mType ) );
    switch ( vitem->mType ) {
    case TReply:
      mList->currentItem()->setIcon( 0, mReplyPix );
      break;
    case TReplyAll:
      mList->currentItem()->setIcon( 0, mReplyAllPix );
      break;
    case TForward:
      mList->currentItem()->setIcon( 0, mForwardPix );
      break;
    default:
      mList->currentItem()->setIcon( 0, QPixmap() );
      break;
    };

    // see slotListSelectionChanged() above
    mKeySequenceWidget->setEnabled( vitem->mType != TUniversal );
    emit changed();
  }
}

void CustomTemplates::slotShortcutChanged( const QKeySequence &newSeq )
{
    if ( mList->currentItem() )
    {
      mItemList[ mList->currentItem()->text( 1 ) ]->mShortcut =
          KShortcut( newSeq );
      mKeySequenceWidget->applyStealShortcut();
    }

    emit changed();
}

#include "customtemplates.moc"
