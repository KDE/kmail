/*
  kmfawidgets.h - KMFilterAction parameter widgets
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

#include "kmsoundtestwidget.h"

#include <QHBoxLayout>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <KUrlRequester>
#include <kfiledialog.h>
#include <QPushButton>
#include <klocalizedstring.h>
#include <phonon/mediaobject.h>

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
