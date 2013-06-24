/*
 * Copyright (c) 2011, 2012, 2013 Montel Laurent <montel@kde.org>
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
#include "kmkernel.h"

#include <mailcommon/tag/addtagdialog.h>

#include <KListWidgetSearchLine>

#include <Nepomuk2/Resource>
#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Tag>
#include <QGridLayout>
#include <QListWidget>
#include <akonadi/item.h>

using namespace KMail;

TagSelectDialog::TagSelectDialog( QWidget * parent, int numberOfSelectedMessages, const Akonadi::Item &selectedItem)
    : KDialog( parent ),
      mNumberOfSelectedMessages(numberOfSelectedMessages),
      mSelectedItem(selectedItem)
{
    setCaption( i18n( "Select Tags" ) );
    setButtons( User1|Ok|Cancel );
    setButtonText(User1, i18n("Add new tag..."));
    setDefaultButton( Ok );
    setModal( true );

    QWidget *mainWidget = new QWidget( this );
    QGridLayout *mainLayout = new QGridLayout( mainWidget );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );
    setMainWidget( mainWidget );

    mListTag = new QListWidget( this );
    KListWidgetSearchLine *listWidgetSearchLine = new KListWidgetSearchLine(this,mListTag);
    listWidgetSearchLine->setClickMessage(i18n("Search tag"));
    listWidgetSearchLine->setClearButtonShown(true);

    mainLayout->addWidget(listWidgetSearchLine);
    mainLayout->addWidget( mListTag );

    createTagList();
    connect(this, SIGNAL(user1Clicked()), SLOT(slotAddNewTag()));

    KConfigGroup group( KMKernel::self()->config(), "TagSelectDialog" );
    const QSize size = group.readEntry( "Size", QSize() );
    if ( size.isValid() ) {
        resize( size );
    } else {
        resize( 500, 300 );
    }
}

TagSelectDialog::~TagSelectDialog()
{
    KConfigGroup group( KMKernel::self()->config(), "TagSelectDialog" );
    group.writeEntry( "Size", size() );
}

void TagSelectDialog::slotAddNewTag()
{
    QPointer<MailCommon::AddTagDialog> dialog = new MailCommon::AddTagDialog(QList<KActionCollection*>(), this);
    dialog->setTags(mTagList);
    if ( dialog->exec() ) {
        mListTag->clear();
        mTagList.clear();
        createTagList();
    }
    delete dialog;
}

void TagSelectDialog::createTagList()
{
    foreach( const Nepomuk2::Tag &nepomukTag, Nepomuk2::Tag::allTags() ) {
        mTagList.append( MailCommon::Tag::fromNepomuk( nepomukTag ) );
    }
    qSort( mTagList.begin(), mTagList.end(), MailCommon::Tag::compare );

    Nepomuk2::Resource itemResource( mSelectedItem.url() );

    foreach( const MailCommon::Tag::Ptr &tag, mTagList ) {
        if(tag->tagStatus)
            continue;
        QListWidgetItem *item = new QListWidgetItem(KIcon(tag->iconName), tag->tagName, mListTag );
        item->setData(UrlTag, tag->nepomukResourceUri.toString());
        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
        item->setCheckState( Qt::Unchecked );
        mListTag->addItem( item );

        if ( mNumberOfSelectedMessages == 1 ) {
            const bool hasTag = itemResource.tags().contains(  Nepomuk2::Tag( tag->tagName ) );
            item->setCheckState( hasTag ? Qt::Checked : Qt::Unchecked );
        } else {
            item->setCheckState( Qt::Unchecked );
        }
    }

}

QList<QString> TagSelectDialog::selectedTag() const
{
    QList<QString> lst;
    const int numberOfItems( mListTag->count() );
    for ( int i = 0; i< numberOfItems;++i ) {
        QListWidgetItem *item = mListTag->item( i );
        if ( item->checkState() == Qt::Checked ) {
            lst.append( item->data(UrlTag).toString() );
        }
    }
    return lst;
}

#include "tagselectdialog.moc"
