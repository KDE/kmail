/*
    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Till Adam <adam@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "config.h"

#include "headerlistquicksearch.h"

#include <qapplication.h>
#include <qlabel.h>
#include <qcombobox.h>

#include <qtimer.h>
//Added by qt3to4:
#include <QEvent>

#include <kaction.h>
#include <kiconloader.h>
#include <k3listview.h>
#include <klocale.h>

#include "kmfolder.h"
#include "kmheaders.h"
#include "kmsearchpattern.h"
#include "kmmainwidget.h"

namespace KMail {

HeaderListQuickSearch::HeaderListQuickSearch( QWidget *parent,
                                              K3ListView *listView,
                                              KActionCollection *actionCollection )
  : K3ListViewSearchLine( parent, listView ), mStatusCombo(0), mStatus(),  statusList()
{
  KAction *resetQuickSearch = new KAction( i18n( "Reset Quick Search" ),
                                           QApplication::isRightToLeft()
                                           ? "clear_left"
                                           : "locationbar_erase",
                                           0, this,
                                           SLOT( reset() ),
                                           actionCollection,
                                           "reset_quicksearch" );
  resetQuickSearch->plug( parent );
  resetQuickSearch->setWhatsThis( i18n( "Reset Quick Search\n"
                                        "Resets the quick search so that "
                                        "all messages are shown again." ) );

  QLabel *label = new QLabel( i18n("Stat&us:"), parent );
  label->setObjectName( "kde toolbar widget" );

  mStatusCombo = new QComboBox( parent );
  mStatusCombo->setObjectName( "quick search status combo box" );
  mStatusCombo->addItem( SmallIcon( "run" ), i18n("Any Status") );

  insertStatus( StatusUnread );
  insertStatus( StatusNew );
  insertStatus( StatusImportant );
  insertStatus( StatusReplied );
  insertStatus( StatusForwarded );
  insertStatus( StatusToDo );
  insertStatus( StatusHasAttachment );
  insertStatus( StatusWatched );
  insertStatus( StatusIgnored );
  mStatusCombo->setCurrentIndex( 0 );
  mStatusCombo->installEventFilter( this );
  connect( mStatusCombo, SIGNAL ( activated( int ) ),
           this, SLOT( slotStatusChanged( int ) ) );

  label->setBuddy( mStatusCombo );

  /* Disable the signal connected by K3ListViewSearchLine since it will call
   * itemAdded during KMHeaders::readSortOrder() which will in turn result
   * in getMsgBaseForItem( item ) wanting to access items which are no longer
   * there. Rather rely on KMHeaders::msgAdded and its signal. */
  disconnect(listView, SIGNAL(itemAdded(Q3ListViewItem *)),
             this, SLOT(itemAdded(Q3ListViewItem *)));
  KMHeaders *headers = static_cast<KMHeaders*>( listView );
  connect( headers, SIGNAL( msgAddedToListView( Q3ListViewItem* ) ),
           this, SLOT( itemAdded( Q3ListViewItem* ) ) );

}

HeaderListQuickSearch::~HeaderListQuickSearch()
{
}


bool HeaderListQuickSearch::eventFilter( QObject *watched, QEvent *event )
{
  if ( watched == mStatusCombo ) {
    KMMainWidget *mainWidget = 0;

    // Travel up the parents list until we find the main widget
    for ( QWidget *curWidget = parentWidget(); curWidget; curWidget = curWidget->parentWidget() ) {
      mainWidget = ::qobject_cast<KMMainWidget *>( curWidget );
      if ( mainWidget )
        break;
    }

    if ( mainWidget ) {
      switch ( event->type() ) {
      case QEvent::FocusIn:
        mainWidget->setAccelsEnabled( false );
        break;
      case QEvent::FocusOut:
        mainWidget->setAccelsEnabled( true );
        break;
      default:
        // Avoid compiler warnings
        break;
      }
    }
  }

  // In either case, always return false, we NEVER want to eat the event
  return false;
}


bool HeaderListQuickSearch::itemMatches(const Q3ListViewItem *item, const QString &s) const
{
  if ( !mStatus.isOfUnknownStatus() ) {
    KMHeaders *headers = static_cast<KMHeaders*>( item->listView() );
    const KMMsgBase *msg = headers->getMsgBaseForItem( item );
    if ( !msg || ! ( msg->messageStatus() & mStatus ) )
      return false;
  }
  return K3ListViewSearchLine::itemMatches(item, s);
}

//-----------------------------------------------------------------------------
void HeaderListQuickSearch::reset()
{
  clear();
  mStatusCombo->setCurrentIndex( 0 );
  slotStatusChanged( 0 );
}

void HeaderListQuickSearch::slotStatusChanged( int index )
{
  if ( index == 0 )
    mStatus.clear();
  else
    mStatus = KMSearchRuleStatus::statusFromEnglishName( statusList[index - 1] );
  updateSearch();
}

void HeaderListQuickSearch::insertStatus(KMail::StatusValueTypes which)
{
  mStatusCombo->addItem( SmallIcon( KMail::StatusValues[which].icon ),
    i18n( KMail::StatusValues[ which ].text ) );
  statusList.append( KMail::StatusValues[ which ].text );
}

} // namespace KMail

#include "headerlistquicksearch.moc"
