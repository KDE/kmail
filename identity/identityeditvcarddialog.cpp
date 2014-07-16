/*
  Copyright (c) 2012-2013 Montel Laurent <montel@kde.org>
  
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

#include "identityeditvcarddialog.h"

#include <KABC/VCardConverter>
#include <KLocalizedString>
#include <Akonadi/Contact/ContactEditor>
#include <KDebug>
#include <KMessageBox>
#include <KStandardDirs>

#include <QHBoxLayout>
#include <QFile>

IdentityEditVcardDialog::IdentityEditVcardDialog(const QString &fileName, QWidget *parent)
    : KDialog(parent)
{
    if (QFile(fileName).exists()) {
        setCaption( i18n( "Edit own vCard" ) );
        setButtons( User1|Ok|Cancel );
        setButtonText(User1, i18n("Delete current vCard"));
        connect(this, SIGNAL(user1Clicked()), this, SLOT(slotDeleteCurrentVCard()));
    } else {
        setCaption( i18n("Create own vCard") );
        setButtons( Ok|Cancel );
    }

    setDefaultButton( Ok );
    setModal( true );
    QWidget *mainWidget = new QWidget( this );
    QHBoxLayout *mainLayout = new QHBoxLayout( mainWidget );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );
    setMainWidget( mainWidget );

    mContactEditor = new Akonadi::ContactEditor( Akonadi::ContactEditor::CreateMode, Akonadi::ContactEditor::VCardMode, this );
    mainLayout->addWidget(mContactEditor);
    loadVcard(fileName);
}

IdentityEditVcardDialog::~IdentityEditVcardDialog()
{
}

void IdentityEditVcardDialog::slotDeleteCurrentVCard()
{
    if (mVcardFileName.isEmpty())
        return;
    if (KMessageBox::Yes == KMessageBox::questionYesNo(this, i18n("Are you sure to want to delete this vCard?"), i18n("Delete vCard"))) {
        if(mVcardFileName.startsWith(KGlobal::dirs()->localkdedir())) {
            deleteCurrentVcard(true);
        } else {
            deleteCurrentVcard(false);
        }
        reject();
    }
}

void IdentityEditVcardDialog::deleteCurrentVcard(bool deleteOnDisk)
{
    if (!mVcardFileName.isEmpty()) {
        if (deleteOnDisk) {
            QFile file(mVcardFileName);
            if (file.exists()) {
                if (!file.remove()) {
                    KMessageBox::error(this, i18n("We cannot delete vCard file."), i18n("Delete vCard"));
                }
            }
        }
        Q_EMIT vcardRemoved();
    }
}

void IdentityEditVcardDialog::loadVcard( const QString &vcardFileName)
{
    if (vcardFileName.isEmpty()) {
        return;
    }
    mVcardFileName = vcardFileName;
    QFile file( vcardFileName );

    if ( file.open( QIODevice::ReadOnly ) ) {
        const QByteArray data = file.readAll();
        file.close();
        if ( !data.isEmpty() ) {
            KABC::VCardConverter converter;
            KABC::Addressee addr = converter.parseVCard( data );
            mContactEditor->setContactTemplate(addr);
        }
    }
}

QString IdentityEditVcardDialog::saveVcard() const
{
    const KABC::Addressee addr = mContactEditor->contact();
    KABC::VCardConverter converter;
    QFile file(mVcardFileName);
    if ( file.open( QIODevice::WriteOnly |QIODevice::Text ) ) {
        const QByteArray data = converter.exportVCard( addr, KABC::VCardConverter::v3_0 );
        file.write( data );
        file.flush();
        file.close();
    } else {
        kDebug()<<"We cannot open file: "<<mVcardFileName;
    }
    return mVcardFileName;
}

