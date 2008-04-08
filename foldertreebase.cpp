/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "foldertreebase.h"

#include "globalsettings.h"
#include "kmfolder.h"
#include "kmfolderdir.h"
#include "kmfoldertree.h"
#include "kmheaders.h"
#include "kmmainwidget.h"
#include "messagecopyhelper.h"

#include <libkdepim/maillistdrag.h>
using KPIM::MailList;

#include <kapplication.h>
#include <kcolorscheme.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kmenu.h>

#include <QtGui/QCursor>


using namespace KMail;

FolderTreeBase::FolderTreeBase(KMMainWidget *mainWidget, QWidget * parent,
                               const char * name)
 : KFolderTree( parent, name ),
   mMainWidget( mainWidget )
{
  addAcceptableDropMimetype( MailList::mimeDataType(), false );
}

void FolderTreeBase::contentsDropEvent( QDropEvent *e )
{
  Q3ListViewItem *item = itemAt( contentsToViewport( e->pos() ) );
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem *>( item );
  if ( fti && fti->folder() && e->mimeData()->hasFormat( MailList::mimeDataType() ) ) {
    if ( e->source() == mMainWidget->headers()->viewport() ) {
      int action;
      if ( mMainWidget->headers()->folder() && mMainWidget->headers()->folder()->isReadOnly() )
        action = DRAG_COPY;
      else
        action = dndMode();
      // KMHeaders does copy/move itself
      if ( action == DRAG_MOVE && fti->folder() )
        emit folderDrop( fti->folder() );
      else if ( action == DRAG_COPY && fti->folder() )
        emit folderDropCopy( fti->folder() );
    } else {
      handleMailListDrop( e, fti->folder() );
    }
    e->setAccepted( true );
  } else {
    KFolderTree::contentsDropEvent( e );
  }
}

int FolderTreeBase::dndMode(bool alwaysAsk)
{
  int action = -1;
  int keybstate = kapp->keyboardModifiers();
  if ( keybstate & Qt::CTRL ) {
    action = DRAG_COPY;
  } else if ( keybstate & Qt::SHIFT ) {
    action = DRAG_MOVE;
  } else {
    if ( GlobalSettings::self()->showPopupAfterDnD() || alwaysAsk ) {
      KMenu menu;
      QAction *moveAction = menu.addAction( SmallIcon( "edit-rename"), i18n( "&Move Here" ) );
      QAction *copyAction = menu.addAction( SmallIcon( "edit-copy" ), i18n( "&Copy Here" ) );
      menu.addSeparator();
      menu.addAction( SmallIcon( "dialog-cancel" ), i18n( "C&ancel" ) );

      QAction *menuChoice = menu.exec( QCursor::pos() );
      if ( menuChoice == moveAction )
        action = DRAG_MOVE;
      else if ( menuChoice == copyAction )
        action = DRAG_COPY;
      else
        action = DRAG_CANCEL;
    }
    else
      action = DRAG_MOVE;
  }
  return action;
}

bool FolderTreeBase::event(QEvent * e)
{
  if ( e->type() == QEvent::ApplicationPaletteChange ) {
     readColorConfig();
     return true;
  }
  return KFolderTree::event(e);
}

void FolderTreeBase::readColorConfig()
{
  KConfigGroup cg = KMKernel::config()->group( "Reader" );

  // Custom/System color support
  QColor c1 = KColorScheme( QPalette::Active, KColorScheme::View ).foreground().color();
  QColor c2 = QColor( "blue" );
  QColor c4 = KColorScheme( QPalette::Active, KColorScheme::View ).background().color();
  QColor c5 = QColor( "red" );

  if ( !cg.readEntry( "defaultColors", true ) ) {
    mPaintInfo.colFore = cg.readEntry( "ForegroundColor", c1 );
    mPaintInfo.colUnread = cg.readEntry( "UnreadMessage", c2 );
    mPaintInfo.colBack = cg.readEntry( "BackgroundColor", c4 );
    mPaintInfo.colCloseToQuota = cg.readEntry( "CloseToQuotaColor", c5 );
  }
  else {
    mPaintInfo.colFore = c1;
    mPaintInfo.colUnread = c2;
    mPaintInfo.colBack = c4;
    mPaintInfo.colCloseToQuota = c5;
  }
  QPalette newPal = kapp->palette();
  newPal.setColor( QColorGroup::Base, mPaintInfo.colBack );
  newPal.setColor( QColorGroup::Text, mPaintInfo.colFore );
  setPalette( newPal );
}

bool FolderTreeBase::hideLocalInbox() const
{
  if ( !GlobalSettings::self()->hideLocalInbox() )
    return false;
  KMFolder *localInbox = kmkernel->inboxFolder();
  assert( localInbox );
  // check if it is empty
  KMFolderOpener openInbox( localInbox, "FolderTreeBase" );
  if ( localInbox->count() > 0 )
    return false;
  // check if it has subfolders
  if ( localInbox->child() && !localInbox->child()->isEmpty() )
    return false;
  // check if it is an account target
  if ( localInbox->hasAccounts() )
    return false;
  return true;
}

void FolderTreeBase::handleMailListDrop(QDropEvent * event, KMFolder *destination )
{
  MailList list = MailList::fromMimeData( event->mimeData() );
  if ( list.isEmpty() ) {
    kWarning() << "Could not decode drag data!";
  } else {
    QList<uint> serNums = MessageCopyHelper::serNumListFromMailList( list );
    int action;
    if ( MessageCopyHelper::inReadOnlyFolder( serNums ) )
      action = DRAG_COPY;
    else
      action = dndMode();
    if ( action == DRAG_COPY || action == DRAG_MOVE ) {
      new MessageCopyHelper( serNums, destination, action == DRAG_MOVE, this );
    }
  }
}

#include "foldertreebase.moc"
