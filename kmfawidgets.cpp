// kmfawidgets.h - KMFilterAction parameter widgets
// Copyright: (c) 2001 Marc Mutz <mutz@kde.org>
// License: GNU Genaral Public License

#include "kmfawidgets.h"

#include <kabc/addresseedialog.h> // for the button in KMFilterActionWithAddress
#include <kiconloader.h>
#include <klocale.h>
#include <kaudioplayer.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>

#include <qlayout.h>

//=============================================================================
//
// class KMFilterActionWithAddressWidget
//
//=============================================================================

KMFilterActionWithAddressWidget::KMFilterActionWithAddressWidget( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  QHBoxLayout *hbl = new QHBoxLayout(this);
  hbl->setSpacing(4);
  mLineEdit = new KLineEdit(this);
  hbl->addWidget( mLineEdit, 1 /*stretch*/ );
  mBtn = new QPushButton( QString::null ,this );
  mBtn->setPixmap( BarIcon( "contents", KIcon::SizeSmall ) );
  mBtn->setFixedHeight( mLineEdit->sizeHint().height() );
  hbl->addWidget( mBtn );

  connect( mBtn, SIGNAL(clicked()),
	   this, SLOT(slotAddrBook()) );
}

void KMFilterActionWithAddressWidget::slotAddrBook()
{
  KABC::Addressee::List lst = KABC::AddresseeDialog::getAddressees( this );

  if ( lst.empty() )
    return;

  QStringList addrList;

  for( KABC::Addressee::List::const_iterator it = lst.begin(); it != lst.end(); ++it )
    addrList << (*it).fullEmail();

  QString txt = mLineEdit->text().stripWhiteSpace();

  if ( !txt.isEmpty() ) {
    if ( !txt.endsWith( "," ) )
      txt += ", ";
    else
      txt += ' ';
  }

  mLineEdit->setText( txt + addrList.join(",") );
}

KMSoundTestWidget::KMSoundTestWidget(QWidget *parent, const char *name)
    : QWidget( parent, name)
{
    QHBoxLayout *lay1 = new QHBoxLayout( this );
    m_playButton = new QPushButton( this, "m_playButton" );
    m_playButton->setPixmap( SmallIcon( "1rightarrow" ) );
    connect( m_playButton, SIGNAL( clicked() ), SLOT( playSound() ));
    lay1->addWidget( m_playButton );

    m_urlRequester = new KURLRequester( this );
    lay1->addWidget( m_urlRequester );
    connect( m_urlRequester, SIGNAL( openFileDialog( KURLRequester * )),
             SLOT( openSoundDialog( KURLRequester * )));
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

void KMSoundTestWidget::openSoundDialog( KURLRequester * )
{
    static bool init = true;
    if ( !init )
        return;

    init = false;

    KFileDialog *fileDialog = m_urlRequester->fileDialog();
    fileDialog->setCaption( i18n("Select Sound File") );
    QStringList filters;
    filters << "audio/x-wav" << "audio/x-mp3" << "application/x-ogg"
            << "audio/x-adpcm";
    fileDialog->setMimeFilter( filters );

   QStringList soundDirs = KGlobal::dirs()->resourceDirs( "sound" );

    if ( !soundDirs.isEmpty() ) {
        KURL soundURL;
        QDir dir;
        dir.setFilter( QDir::Files | QDir::Readable );
        QStringList::ConstIterator it = soundDirs.begin();
        while ( it != soundDirs.end() ) {
            dir = *it;
            if ( dir.isReadable() && dir.count() > 2 ) {
                soundURL.setPath( *it );
                fileDialog->setURL( soundURL );
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
    KAudioPlayer::play(QFile::encodeName(play));
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
