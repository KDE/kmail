/*
  kmfawidgets.cpp - KMFilterAction parameter widgets
  Copyright (c) 2001 Marc Mutz <mutz@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kmfawidgets.h"

#include <kabc/addresseedialog.h> // for the button in KMFilterActionWithAddress
#include <kiconloader.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <Phonon/MediaObject>

#include <QHBoxLayout>

//=============================================================================
//
// class KMFilterActionWithAddressWidget
//
//=============================================================================

KMFilterActionWithAddressWidget::KMFilterActionWithAddressWidget( QWidget* parent, const char* name )
  : QWidget( parent )
{
  setObjectName( name );
  QHBoxLayout *hbl = new QHBoxLayout(this);
  hbl->setSpacing(4);
  hbl->setMargin( 0 );
  mLineEdit = new KLineEdit(this);
  mLineEdit->setObjectName( "addressEdit" );
  hbl->addWidget( mLineEdit, 1 /*stretch*/ );
  mBtn = new QPushButton( QString(),this );
  mBtn->setIcon( KIcon( "help-contents" ) );
  mBtn->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
  mBtn->setFixedHeight( mLineEdit->sizeHint().height() );
  mBtn->setToolTip( i18n( "Open Address Book" ) );
  hbl->addWidget( mBtn );

  connect( mBtn, SIGNAL(clicked()), this, SLOT(slotAddrBook()) );
}

void KMFilterActionWithAddressWidget::slotAddrBook()
{
  KABC::Addressee::List lst = KABC::AddresseeDialog::getAddressees( this );

  if ( lst.empty() )
    return;

  QStringList addrList;

  for( KABC::Addressee::List::const_iterator it = lst.constBegin(); it != lst.constEnd(); ++it )
    addrList << (*it).fullEmail();

  QString txt = mLineEdit->text().trimmed();

  if ( !txt.isEmpty() ) {
    if ( !txt.endsWith( ',' ) )
      txt += ", ";
    else
      txt += ' ';
  }

  mLineEdit->setText( txt + addrList.join(",") );
}

KMSoundTestWidget::KMSoundTestWidget(QWidget *parent, const char *name)
    : QWidget( parent )
{
    setObjectName( name );
    QHBoxLayout *lay1 = new QHBoxLayout( this );
    lay1->setMargin( 0 );
    m_playButton = new QPushButton( this );
    m_playButton->setObjectName( "m_playButton" );
    m_playButton->setIcon( KIcon( "arrow-right" ) );
    m_playButton->setIconSize( QSize( KIconLoader::SizeSmall, KIconLoader::SizeSmall ) );
    connect( m_playButton, SIGNAL( clicked() ), SLOT( playSound() ));
    lay1->addWidget( m_playButton );

    m_urlRequester = new KUrlRequester( this );
    lay1->addWidget( m_urlRequester );
    connect( m_urlRequester, SIGNAL( openFileDialog( KUrlRequester * )),
             SLOT( openSoundDialog( KUrlRequester * )));
    connect( m_urlRequester->lineEdit(), SIGNAL( textChanged ( const QString & )), SLOT( slotUrlChanged(const QString & )));
    slotUrlChanged(m_urlRequester->lineEdit()->text() );
}

KMSoundTestWidget::~KMSoundTestWidget()
{
}

void KMSoundTestWidget::slotUrlChanged(const QString &_text )
{
    m_playButton->setEnabled( !_text.isEmpty());
}

void KMSoundTestWidget::openSoundDialog( KUrlRequester * )
{
    static bool init = true;
    if ( !init )
        return;

    init = false;

    KFileDialog *fileDialog = m_urlRequester->fileDialog();
    fileDialog->setCaption( i18n("Select Sound File") );
    QStringList filters;
    filters << "audio/x-wav" << "audio/mpeg" << "application/ogg"
            << "audio/x-adpcm";
    fileDialog->setMimeFilter( filters );

   const QStringList soundDirs = KGlobal::dirs()->resourceDirs( "sound" );

    if ( !soundDirs.isEmpty() ) {
        KUrl soundURL;
        QDir dir;
        dir.setFilter( QDir::Files | QDir::Readable );
        QStringList::ConstIterator it = soundDirs.constBegin();
        while ( it != soundDirs.constEnd() ) {
            dir = *it;
            if ( dir.isReadable() && dir.count() > 2 ) {
                soundURL.setPath( *it );
                fileDialog->setUrl( soundURL );
                break;
            }
            ++it;
        }
    }

}

void KMSoundTestWidget::playSound()
{
    QString parameter= m_urlRequester->lineEdit()->text();
    if ( parameter.isEmpty() )
        return ;
    QString play = parameter;
    QString file = QString::fromLatin1("file:");
    if (parameter.startsWith(file))
        play = parameter.mid(file.length());
    Phonon::MediaObject* player = Phonon::createPlayer( Phonon::NotificationCategory, play );
    player->play();
    connect( player, SIGNAL( finished() ), player, SLOT( deleteLater() ) );
}


QString KMSoundTestWidget::url() const
{
    return m_urlRequester->lineEdit()->text();
}

void KMSoundTestWidget::setUrl(const QString & url)
{
    m_urlRequester->lineEdit()->setText(url);
}

void KMSoundTestWidget::clear()
{
    m_urlRequester->lineEdit()->clear();
}

//--------------------------------------------
#include "kmfawidgets.moc"
