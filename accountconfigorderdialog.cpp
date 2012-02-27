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

#include "mailcommon/mailutil.h"


#include <KLocale>
#include <KPushButton>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>

#include <KMime/KMimeMessage>

#include <QVBoxLayout>
#include <QListWidget>

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
    QVBoxLayout * vlay = new QVBoxLayout( page );
    mListAccount = new QListWidget(this);
    mListAccount->setDragDropMode( QAbstractItemView::InternalMove );
    vlay->addWidget(mListAccount);

    QHBoxLayout *layout = new QHBoxLayout;
    mUpButton = new KPushButton;
    mUpButton->setIcon( KIcon("go-up") );
    mUpButton->setEnabled( false ); // b/c no item is selected yet
    mUpButton->setFocusPolicy( Qt::StrongFocus );

    mDownButton = new KPushButton;
    mDownButton->setIcon( KIcon("go-down") );
    mDownButton->setEnabled( false ); // b/c no item is selected yet
    mDownButton->setFocusPolicy( Qt::StrongFocus );
    layout->addWidget(mUpButton);
    layout->addWidget(mDownButton);

    vlay->addLayout(layout);

    connect( mUpButton, SIGNAL(clicked()), this, SLOT(slotMoveUp()) );
    connect( mDownButton, SIGNAL(clicked()), this, SLOT(slotMoveDown()) );

    connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
    init();
}

AccountConfigOrderDialog::~AccountConfigOrderDialog()
{

}

void AccountConfigOrderDialog::slotMoveUp()
{

}

void AccountConfigOrderDialog::slotMoveDown()
{

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
                if(!instance.identifier().contains(POP3_RESOURCE_IDENTIFIER)) {
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
    for(int i = 0; i< numberOfList;++i) {
        instanceList.removeOne(listOrderAccount.at(i));
    }

    QStringList finalList (listOrderAccount);
    finalList += instanceList;

    const int numberOfElement(finalList.count());
    for(int i = 0; i <numberOfElement;++i) {
        const QString identifier(finalList.at(i));
        if(mapInstance.contains(identifier)) {
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
    for(int i = 0; i<numberOfItems; ++i) {
        order<<mListAccount->item(i)->data(AccountConfigOrderDialog::IdentifierAccount).toString();
    }
    KConfigGroup group( KMKernel::self()->config(), "AccountOrder" );
    group.writeEntry("order",order);
    group.sync();

}

#include "accountconfigorderdialog.moc"
