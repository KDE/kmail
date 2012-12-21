/*
 * Copyright (c) 2011 Montel Laurent <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "tagselectdialog.h"
#include "tag.h"

#include <KListWidgetSearchLine>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Tag>
#include <QGridLayout>
#include <QListWidget>
#include <akonadi/item.h>

using namespace KMail;

TagSelectDialog::TagSelectDialog( QWidget * parent, int numberOfSelectedMessages, const Akonadi::Item &selectedItem)
  : KDialog( parent )
{
  setCaption( i18n( "Select Tags" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  
  QWidget *mainWidget = new QWidget( this );
  QGridLayout *mainLayout = new QGridLayout( mainWidget );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );
  setMainWidget( mainWidget );

  mListTag = new QListWidget( this );
  mListWidgetSearchLine = new KListWidgetSearchLine(this,mListTag);
  mListWidgetSearchLine->setClickMessage(i18n("Search tag"));
  mListWidgetSearchLine->setClearButtonShown(true);

  mainLayout->addWidget(mListWidgetSearchLine);
  mainLayout->addWidget( mListTag );
  
  QList<Tag::Ptr> tagList;
  foreach( const Nepomuk::Tag &nepomukTag, Nepomuk::Tag::allTags() ) {
    tagList.append( Tag::fromNepomuk( nepomukTag ) );
  }
  qSort( tagList.begin(), tagList.end(), KMail::Tag::compare );

  Nepomuk::Resource itemResource( selectedItem.url() );

  foreach( const Tag::Ptr &tag, tagList ) {
    QListWidgetItem *item = new QListWidgetItem( tag->tagName, mListTag );
    item->setData(UrlTag, tag->nepomukResourceUri.toString());
    item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    item->setCheckState( Qt::Unchecked );
    mListTag->addItem( item );

    if ( numberOfSelectedMessages == 1 ) {
      const bool hasTag = itemResource.tags().contains(  Nepomuk::Tag( tag->tagName ) );
      item->setCheckState( hasTag ? Qt::Checked : Qt::Unchecked );
    } else {
      item->setCheckState( Qt::Unchecked );
    }    
  }
}

TagSelectDialog::~TagSelectDialog()
{
  
}

QList<QString> TagSelectDialog::selectedTag() const
{
  QList<QString> lst;
  const int numberOfItems( mListTag->count() );
  for ( int i = 0; i< numberOfItems;++i )
  {
    QListWidgetItem *item = mListTag->item( i );
    if ( item->checkState() == Qt::Checked )
    {
      lst.append( item->data(UrlTag).toString() );
    }
  }
  return lst;
}

#include "tagselectdialog.moc"
