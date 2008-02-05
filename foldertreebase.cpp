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
#include "folderstorage.h"

#include <libkdepim/maillistdrag.h>
using KPIM::MailList;
using KPIM::MailListDrag;

#include <kconfig.h>
#include <kiconloader.h>
#include <kpopupmenu.h>

#include <qcursor.h>

using namespace KMail;

FolderTreeBase::FolderTreeBase(KMMainWidget *mainWidget, QWidget * parent, const char * name) :
    KFolderTree( parent, name ),
    mMainWidget( mainWidget )
{
  addAcceptableDropMimetype(MailListDrag::format(), false);
}

void FolderTreeBase::contentsDropEvent(QDropEvent * e)
{
  QListViewItem *item = itemAt( contentsToViewport(e->pos()) );
  KMFolderTreeItem *fti = static_cast<KMFolderTreeItem*>(item);
  if ( fti && fti->folder() && e->provides( MailListDrag::format() ) ) {
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
    e->accept( true );
  } else {
    KFolderTree::contentsDropEvent( e );
  }
}

int FolderTreeBase::dndMode(bool alwaysAsk)
{
  int action = -1;
  int keybstate = kapp->keyboardModifiers();
  if ( keybstate & KApplication::ControlModifier ) {
    action = DRAG_COPY;
  } else if ( keybstate & KApplication::ShiftModifier ) {
    action = DRAG_MOVE;
  } else {
    if ( GlobalSettings::self()->showPopupAfterDnD() || alwaysAsk ) {
      KPopupMenu *menu = new KPopupMenu( this );
      menu->insertItem( i18n("&Move Here"), DRAG_MOVE, 0 );
      menu->insertItem( SmallIcon("editcopy"), i18n("&Copy Here"), DRAG_COPY, 1 );
      menu->insertSeparator();
      menu->insertItem( SmallIcon("cancel"), i18n("C&ancel"), DRAG_CANCEL, 3 );
      action = menu->exec( QCursor::pos(), 0 );
    }
    else
      action = DRAG_MOVE;
  }
  return action;
}

bool FolderTreeBase::event(QEvent * e)
{
  if (e->type() == QEvent::ApplicationPaletteChange) {
     readColorConfig();
     return true;
  }
  return KFolderTree::event(e);
}

void FolderTreeBase::readColorConfig()
{
  KConfig* conf = KMKernel::config();
  // Custom/System color support
  KConfigGroupSaver saver(conf, "Reader");
  QColor c1=QColor(kapp->palette().active().text());
  QColor c2=QColor("blue");
  QColor c4=QColor(kapp->palette().active().base());
  QColor c5=QColor("red");

  if (!conf->readBoolEntry("defaultColors",true)) {
    mPaintInfo.colFore = conf->readColorEntry("ForegroundColor",&c1);
    mPaintInfo.colUnread = conf->readColorEntry("UnreadMessage",&c2);
    mPaintInfo.colBack = conf->readColorEntry("BackgroundColor",&c4);
    mPaintInfo.colCloseToQuota = conf->readColorEntry("CloseToQuotaColor",&c5);
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
  localInbox->open( "foldertreebase" );
  if ( localInbox->count() > 0 ) {
    localInbox->close( "foldertreebase" );
    return false;
  }
  localInbox->close( "foldertreebase" );
  // check if it has subfolders
  if ( localInbox->child() && !localInbox->child()->isEmpty() )
    return false;
  // check if it is an account target
  if ( localInbox->hasAccounts() )
    return false;
  return true;
}


void FolderTreeBase::slotUpdateCounts(KMFolder * folder, bool force /* = false*/)
{
 // kdDebug(5006) << "KMFolderTree::slotUpdateCounts()" << endl;
  QListViewItem * current;
  if (folder)
    current = indexOfFolder(folder);
  else
    current = currentItem();

  KMFolderTreeItem* fti = static_cast<KMFolderTreeItem*>(current);

  // sanity check
  if (!fti) return;
  if (!fti->folder()) fti->setTotalCount(-1);

  // get the unread count
  int count = 0;
  if (folder && folder->noContent()) // always empty
    count = -1;
  else {
    if ( fti->folder() )
      count = fti->folder()->countUnread();
  }

  // set it
  bool repaint = false;
  if (fti->unreadCount() != count) {
     fti->adjustUnreadCount( count );
     repaint = true;
  }
  if (isTotalActive() || force)
  {
    // get the total-count
    if (fti->folder()->noContent())
      count = -1;
    else {
      // get the cached count if the folder is not open
      count = fti->folder()->count( !fti->folder()->isOpened() );
    }
    // set it
    if ( count != fti->totalCount() ) {
      fti->setTotalCount(count);
      repaint = true;
    }
  }
  if ( isSizeActive() || force ) {
    if ( !fti->folder()->noContent()) {
      Q_INT64 size = folder->storage()->folderSize();
      if ( size != fti->folderSize() ) {
        fti->setFolderSize( size );
        repaint = true;
      }
    }
  }
  if ( fti->folderIsCloseToQuota() != folder->storage()->isCloseToQuota() ) {
      fti->setFolderIsCloseToQuota( folder->storage()->isCloseToQuota() );
  }

  if (fti->parent() && !fti->parent()->isOpen())
    repaint = false; // we're not visible
  if (repaint) {
    fti->setNeedsRepaint( true );
    emit triggerRefresh();
  }
  // tell the kernel that one of the counts has changed
  kmkernel->messageCountChanged();
}

void FolderTreeBase::handleMailListDrop(QDropEvent * event, KMFolder *destination )
{
  MailList list;
  if ( !MailListDrag::decode( event, list ) ) {
    kdWarning() << k_funcinfo << "Could not decode drag data!" << endl;
  } else {
    QValueList<Q_UINT32> serNums = MessageCopyHelper::serNumListFromMailList( list );
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
