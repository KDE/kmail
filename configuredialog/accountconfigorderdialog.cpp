/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "accountconfigorderdialog.h"
#include "kmkernel.h"

#include "mailcommon/util/mailutil.h"


#include <KLocale>
#include <KPushButton>
#include <KVBox>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>

#include <KMime/KMimeMessage>

#include <QListWidget>
#include <QHBoxLayout>
#include <QDebug>

using namespace KMail;

struct InstanceStruct {
    QString name;
    QIcon icon;
};


AccountConfigOrderDialog::AccountConfigOrderDialog(QWidget *parent)
    :KDialog(parent)
{
    setCaption( i18n("Edit Accounts Order") );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );

    QWidget *page = new QWidget( this );
    setMainWidget( page );
    QHBoxLayout * vlay = new QHBoxLayout( page );
    mListAccount = new QListWidget(this);
    mListAccount->setDragDropMode( QAbstractItemView::InternalMove );
    vlay->addWidget(mListAccount);

    KVBox* upDownBox = new KVBox( page );
    mUpButton = new KPushButton( upDownBox );
    mUpButton->setIcon( KIcon("go-up") );
    mUpButton->setToolTip( i18nc( "Move selected account up.", "Up" ) );
    mUpButton->setEnabled( false ); // b/c no item is selected yet
    mUpButton->setFocusPolicy( Qt::StrongFocus );

    mDownButton = new KPushButton( upDownBox );
    mDownButton->setIcon( KIcon("go-down") );
    mDownButton->setToolTip( i18nc( "Move selected account down.", "Down" ) );
    mDownButton->setEnabled( false ); // b/c no item is selected yet
    mDownButton->setFocusPolicy( Qt::StrongFocus );

    QWidget* spacer = new QWidget( upDownBox );
    upDownBox->setStretchFactor( spacer, 100 );
    vlay->addWidget( upDownBox );


    connect( mUpButton, SIGNAL(clicked()), this, SLOT(slotMoveUp()) );
    connect( mDownButton, SIGNAL(clicked()), this, SLOT(slotMoveDown()) );
    connect( mListAccount, SIGNAL(itemSelectionChanged()), this, SLOT(slotEnableControls()));
    connect( mListAccount->model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),SLOT(slotEnableControls()) );

    connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
    init();
}

AccountConfigOrderDialog::~AccountConfigOrderDialog()
{

}

void AccountConfigOrderDialog::slotMoveUp()
{
  if ( !mListAccount->currentItem() )
    return;
  const int pos = mListAccount->row( mListAccount->currentItem() );
  mListAccount->blockSignals( true );
  QListWidgetItem *item = mListAccount->takeItem( pos );
  // now selected item is at idx(idx-1), so
  // insert the other item at idx, ie. above(below).
  mListAccount->insertItem( pos -1,  item );
  mListAccount->blockSignals( false );
  mListAccount->setCurrentRow( pos - 1 );
}

void AccountConfigOrderDialog::slotMoveDown()
{
  if ( !mListAccount->currentItem() )
    return;
  const int pos = mListAccount->row( mListAccount->currentItem() );
  mListAccount->blockSignals( true );
  QListWidgetItem *item = mListAccount->takeItem( pos );
  // now selected item is at idx(idx-1), so
  // insert the other item at idx, ie. above(below).
  mListAccount->insertItem( pos +1 , item );
  mListAccount->blockSignals( false );
  mListAccount->setCurrentRow( pos + 1 );
}


void AccountConfigOrderDialog::slotEnableControls()
{
  QListWidgetItem *item = mListAccount->currentItem();

  mUpButton->setEnabled( item && mListAccount->currentRow()!=0 );
  mDownButton->setEnabled( item && mListAccount->currentRow()!=mListAccount->count()-1 );
}

void AccountConfigOrderDialog::init()
{
    KConfigGroup group( KMKernel::self()->config(), "AccountOrder" );
    const QStringList listOrderAccount = group.readEntry("order",QStringList());
    QStringList instanceList;

    QMap<QString, InstanceStruct> mapInstance;
    foreach ( const Akonadi::AgentInstance &instance, Akonadi::AgentManager::self()->instances() ) {
        const QStringList capabilities( instance.type().capabilities() );
        if ( instance.type().mimeTypes().contains( KMime::Message::mimeType() ) ) {
            if ( capabilities.contains( "Resource" ) &&
                 !capabilities.contains( "Virtual" ) &&
                 !capabilities.contains( "MailTransport" ) )
            {
                if (!instance.identifier().contains(POP3_RESOURCE_IDENTIFIER)) {
                    instanceList<<instance.identifier();
                    InstanceStruct instanceStruct;
                    instanceStruct.name = instance.name();
                    instanceStruct.icon = instance.type().icon();
                    mapInstance.insert(instance.identifier(),instanceStruct);
                }
            }
        }
    }
    const int numberOfList(listOrderAccount.count());
    for (int i = 0; i< numberOfList;++i) {
        instanceList.removeOne(listOrderAccount.at(i));
    }

    QStringList finalList (listOrderAccount);
    finalList += instanceList;

    const int numberOfElement(finalList.count());
    for (int i = 0; i <numberOfElement;++i) {
        const QString identifier(finalList.at(i));
        if (mapInstance.contains(identifier)) {
            InstanceStruct tmp = mapInstance.value(identifier);
            QListWidgetItem * item = new QListWidgetItem(tmp.icon,tmp.name,mListAccount);
            item->setData(AccountConfigOrderDialog::IdentifierAccount, identifier);
            mListAccount->addItem( item );
        }
    }
}

void AccountConfigOrderDialog::slotOk()
{
    QStringList order;
    const int numberOfItems(mListAccount->count());
    for (int i = 0; i<numberOfItems; ++i) {
        order<<mListAccount->item(i)->data(AccountConfigOrderDialog::IdentifierAccount).toString();
    }
    KConfigGroup group( KMKernel::self()->config(), "AccountOrder" );
    group.writeEntry("order",order);
    group.sync();
    KDialog::accept();
}

#include "accountconfigorderdialog.moc"
