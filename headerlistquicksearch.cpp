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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#include "headerlistquicksearch.h"

#include <qapplication.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qtimer.h>

#include <kaction.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>

#include "kmheaders.h"
#include "kmsearchpattern.h"

namespace KMail {

HeaderListQuickSearch::HeaderListQuickSearch( QWidget *parent,
                                              KListView *listView,
                                              KActionCollection *actionCollection,
                                              const char *name )
  : KListViewSearchLine(parent, listView, name), mStatusCombo(0), mStatus(0)
{
  KAction *resetQuickSearch = new KAction( i18n( "Reset Quick Search" ),
                                           QApplication::reverseLayout()
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

  QLabel *label = new QLabel( i18n("Stat&us:"), parent, "kde toolbar widget" );

  mStatusCombo = new QComboBox( parent, "quick search status combo box" );
  mStatusCombo->insertItem( SmallIcon( "run" ), i18n("Any Status") );
  for ( int i = 0; i < KMail::StatusValueCount; i++ )
    mStatusCombo->insertItem( SmallIcon( KMail::StatusValues[ i ].icon ), i18n( KMail::StatusValues[ i ].text ) );
  mStatusCombo->setCurrentItem( 0 );
  connect( mStatusCombo, SIGNAL ( activated( int ) ),
           this, SLOT( slotStatusChanged( int ) ) );

  label->setBuddy( mStatusCombo );

  /* Disable the signal connected by KListViewSearchLine since it will call 
   * itemAdded during KMHeaders::readSortOrder() which will in turn result
   * in getMsgBaseForItem( item ) wanting to access items which are no longer
   * there. Rather rely on KMHeaders::msgAdded and its signal. */
  disconnect(listView, SIGNAL(itemAdded(QListViewItem *)),
             this, SLOT(itemAdded(QListViewItem *)));
  KMHeaders *headers = static_cast<KMHeaders*>( listView );
  connect( headers, SIGNAL( itemAddedToListView( QListViewItem * ) ),
           this, SLOT( itemAdded( QListViewItem* ) ) );

}

HeaderListQuickSearch::~HeaderListQuickSearch()
{
}


bool HeaderListQuickSearch::itemMatches(const QListViewItem *item, const QString &s) const
{
  if ( mStatus != 0 ) {
    KMHeaders *headers = static_cast<KMHeaders*>( item->listView() );
    const KMMsgBase *msg = headers->getMsgBaseForItem( item );
    if ( !msg || ! ( msg->status() & mStatus ) )
      return false;
  }
  return KListViewSearchLine::itemMatches(item, s);
}

//-----------------------------------------------------------------------------
void HeaderListQuickSearch::reset()
{
  clear();
  mStatusCombo->setCurrentItem( 0 );
  slotStatusChanged( 0 );
}

void HeaderListQuickSearch::slotStatusChanged( int index )
{
  if ( index == 0 )
    mStatus = 0;
  else
    mStatus =
      KMSearchRuleStatus::statusFromEnglishName( KMail::StatusValues[ index - 1 ].text );
  updateSearch();
}

} // namespace KMail

#include "headerlistquicksearch.moc"
