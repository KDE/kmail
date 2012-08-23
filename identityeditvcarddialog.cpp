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

#include "identityeditvcarddialog.h"

#include <KABC/VCardConverter>
#include <KLocale>
#include <Akonadi/Contact/ContactEditor>

#include <QHBoxLayout>
#include <QFile>
#include <QDebug>

IdentityEditVcardDialog::IdentityEditVcardDialog(QWidget *parent)
  : KDialog(parent)
{
  setCaption( i18n( "Edit own vcard" ) );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  setModal( true );
  QWidget *mainWidget = new QWidget( this );
  QHBoxLayout *mainLayout = new QHBoxLayout( mainWidget );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );
  setMainWidget( mainWidget );

  mContactEditor = new Akonadi::ContactEditor( Akonadi::ContactEditor::CreateMode, this );
  mainLayout->addWidget(mContactEditor);
}

IdentityEditVcardDialog::~IdentityEditVcardDialog()
{

}


void IdentityEditVcardDialog::loadVcard( const QString& vcardFileName)
{
  if(vcardFileName.isEmpty()) {
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

QString IdentityEditVcardDialog::saveVcard()
{
  KABC::Addressee addr = mContactEditor->contact();
  KABC::VCardConverter converter;
  QFile file(mVcardFileName);
  qDebug()<<" file.filename"<<file.fileName();
  if ( file.open( QIODevice::WriteOnly |QIODevice::Text ) ) {
    const QByteArray data = converter.exportVCard( addr, KABC::VCardConverter::v3_0 );
    file.write( data );
    file.flush();
    file.close();
  }
  return mVcardFileName;
}
