/*
    This file is part of KMail.

    Copyright (c) 2004 Jakob Schr�er <js@camaya.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

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

#include "xfaceconfigurator.h"

#include <akonadi/contact/contactsearchjob.h>
#include <kcombobox.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <kimageio.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include "pimcommon/plaintexteditor/plaintexteditor.h"
#include <kurl.h>
#include <messageviewer/header/kxface.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

// #include <assert.h>
using namespace KABC;
using namespace KIO;
using namespace KMail;
using namespace MessageViewer;

namespace KMail {

  XFaceConfigurator::XFaceConfigurator( QWidget * parent )
    : QWidget( parent )
  {
    // tmp. vars:
    QLabel * label;
    QLabel * label1;
    QLabel * label2;
    QWidget * page;
    QVBoxLayout * vlay;
    QHBoxLayout * hlay;
    QVBoxLayout * page_vlay;
    QPushButton * mFromFileBtn;
    QPushButton * mFromAddrbkBtn;

    vlay = new QVBoxLayout( this );
    vlay->setObjectName( QLatin1String("main layout") );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );
    hlay = new QHBoxLayout();
    vlay->addLayout( hlay );

    // "enable X-Face" checkbox:
    mEnableCheck = new QCheckBox( i18n("&Send picture with every message"), this );
    mEnableCheck->setWhatsThis(
        i18n( "Check this box if you want KMail to add a so-called X-Face header to messages "
            "written with this identity. An X-Face is a small (48x48 pixels) black and "
            "white image that some mail clients are able to display." ) );
    hlay->addWidget( mEnableCheck, Qt::AlignLeft | Qt::AlignVCenter );

    mXFaceLabel = new QLabel( this );
    mXFaceLabel->setWhatsThis(
                    i18n( "This is a preview of the picture selected/entered below." ) );
    mXFaceLabel->setFixedSize(48, 48);
    mXFaceLabel->setFrameShape( QFrame::Box );
    hlay->addWidget( mXFaceLabel );

//     label1 = new QLabel( "X-Face:", this );
//     vlay->addWidget( label1 );

    // "obtain X-Face from" combo and label:
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    KComboBox *sourceCombo = new KComboBox( this );
    sourceCombo->setEditable( false );
    sourceCombo->setWhatsThis(
                    i18n("Click on the widgets below to obtain help on the input methods."));
    sourceCombo->setEnabled( false ); // since !mEnableCheck->isChecked()
    sourceCombo->addItems( QStringList()
        << i18nc( "continuation of \"obtain picture from\"",
                 "External Source" )
        << i18nc( "continuation of \"obtain picture from\"",
                 "Input Field Below" ) );
    label = new QLabel( i18n("Obtain pic&ture from:"), this );
    label->setBuddy( sourceCombo );
    label->setEnabled( false ); // since !mEnableCheck->isChecked()
    hlay->addWidget( label );
    hlay->addWidget( sourceCombo, 1 );

    // widget stack that is controlled by the source combo:
    QStackedWidget * widgetStack = new QStackedWidget( this );
    widgetStack->setEnabled( false ); // since !mEnableCheck->isChecked()
    vlay->addWidget( widgetStack, 1 );
    connect( sourceCombo, SIGNAL(highlighted(int)),
             widgetStack, SLOT(setCurrentIndex(int)) );
    connect( sourceCombo, SIGNAL(activated(int)),
             widgetStack, SLOT(setCurrentIndex(int)) );
    connect( mEnableCheck, SIGNAL(toggled(bool)),
             sourceCombo, SLOT(setEnabled(bool)) );
    connect( mEnableCheck, SIGNAL(toggled(bool)),
             widgetStack, SLOT(setEnabled(bool)) );
    connect( mEnableCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    // The focus might be still in the widget that is disabled
    connect( mEnableCheck, SIGNAL(clicked()),
             mEnableCheck, SLOT(setFocus()) );

    int pageno = 0;
    // page 0: create X-Face from image file or address book entry
    page = new QWidget( widgetStack );
    widgetStack->insertWidget( pageno, page ); // force sequential numbers (play safe)
    page_vlay = new QVBoxLayout( page );
    page_vlay->setMargin( 0 );
    page_vlay->setSpacing( KDialog::spacingHint() );
    hlay = new QHBoxLayout(); // inherits spacing ??? FIXME really?
    page_vlay->addLayout( hlay );
    mFromFileBtn = new QPushButton( i18n("Select File..."), page );
    mFromFileBtn->setWhatsThis(
                     i18n("Use this to select an image file to create the picture from. "
                         "The image should be of high contrast and nearly quadratic shape. "
                         "A light background helps improve the result." ) );
    mFromFileBtn->setAutoDefault( false );
    page_vlay->addWidget( mFromFileBtn, 1 );
    connect( mFromFileBtn, SIGNAL(released()),
             this, SLOT(slotSelectFile()) );
    mFromAddrbkBtn = new QPushButton( i18n("Set From Address Book"), page );
    mFromAddrbkBtn->setWhatsThis(
                     i18n( "You can use a scaled-down version of the picture "
                         "you have set in your address book entry." ) );
    mFromAddrbkBtn->setAutoDefault( false );
    page_vlay->addWidget( mFromAddrbkBtn, 1 );
    connect( mFromAddrbkBtn, SIGNAL(released()),
             this, SLOT(slotSelectFromAddressbook()) );
    label1 = new QLabel( i18n("<qt>KMail can send a small (48x48 pixels), low-quality, "
        "monochrome picture with every message. "
        "For example, this could be a picture of you or a glyph. "
        "It is shown in the recipient's mail client (if supported).</qt>" ), page );
    label1->setAlignment( Qt::AlignVCenter );
    label1->setWordWrap( true );
    page_vlay->addWidget( label1 );

    widgetStack->setCurrentIndex( 0 ); // since sourceCombo->currentItem() == 0

    // page 1: input field for direct entering
    ++pageno;
    page = new QWidget( widgetStack );
    widgetStack->insertWidget( pageno,page );
    page_vlay = new QVBoxLayout( page );
    page_vlay->setMargin( 0 );
    page_vlay->setSpacing( KDialog::spacingHint() );
    mTextEdit = new PimCommon::PlainTextEditor( page );
    page_vlay->addWidget( mTextEdit );
    mTextEdit->setWhatsThis( i18n( "Use this field to enter an arbitrary X-Face string." ) );
    mTextEdit->setFont( KGlobalSettings::fixedFont() );
    mTextEdit->setWordWrapMode( QTextOption::WrapAnywhere);
    mTextEdit->setSearchSupport(false);
    label2 = new QLabel( i18n("Examples are available at <a "
            "href=\"http://ace.home.xs4all.nl/X-Faces/\">"
            "http://ace.home.xs4all.nl/X-Faces/</a>."), page );
    label2->setOpenExternalLinks(true);
    label2->setTextInteractionFlags(Qt::TextBrowserInteraction);

    page_vlay->addWidget( label2 );


    connect(mTextEdit, SIGNAL(textChanged()), this, SLOT(slotUpdateXFace()));
  }

  XFaceConfigurator::~XFaceConfigurator()
  {

  }

  bool XFaceConfigurator::isXFaceEnabled() const
  {
    return mEnableCheck->isChecked();
  }

  void XFaceConfigurator::setXFaceEnabled( bool enable )
  {
    mEnableCheck->setChecked( enable );
  }

  QString XFaceConfigurator::xface() const
  {
    return mTextEdit->toPlainText();
  }

  void XFaceConfigurator::setXFace( const QString & text )
  {
    mTextEdit->setPlainText( text );
  }

  void XFaceConfigurator::setXfaceFromFile( const KUrl &url )
  {
    QString tmpFile;
    if (KIO::NetAccess::download( url, tmpFile, this )) {
      KXFace xf;
      mTextEdit->setPlainText( xf.fromImage( QImage( tmpFile ) ) );
      KIO::NetAccess::removeTempFile( tmpFile );
    } else {
      KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
    }
  }

  void XFaceConfigurator::slotSelectFile()
  {
    const QStringList mimeTypes = KImageIO::mimeTypes (KImageIO::Reading);
    const QString filter = mimeTypes.join (QLatin1String(" "));
    const KUrl url = KFileDialog::getOpenUrl( QString(), filter, this, QString() );
    if ( !url.isEmpty() )
      setXfaceFromFile( url );
  }

  void XFaceConfigurator::slotSelectFromAddressbook()
  {
    using namespace KPIMIdentities;

    IdentityManager manager( true );
    const Identity defaultIdentity = manager.defaultIdentity();
    const QString email = defaultIdentity.primaryEmailAddress().toLower();

    Akonadi::ContactSearchJob *job = new Akonadi::ContactSearchJob( this );
    job->setLimit( 1 );
    job->setQuery( Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch );
    connect( job, SIGNAL(result(KJob*)), SLOT(slotDelayedSelectFromAddressbook(KJob*)) );
  }

  void XFaceConfigurator::slotDelayedSelectFromAddressbook( KJob *job )
  {
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob*>( job );

    if ( searchJob->contacts().isEmpty() ) {
      KMessageBox::information( this, i18n("You do not have your own contact defined in the address book."), i18n("No Picture") );
      return;
    }

    const Addressee contact = searchJob->contacts().first();
    if ( contact.photo().isIntern() )
    {
      const QImage photo = contact.photo().data();
      if ( !photo.isNull() ) {
        KXFace xf;
        mTextEdit->setPlainText( xf.fromImage( photo ) );
      } else {
        KMessageBox::information( this, i18n("No picture set for your address book entry."), i18n("No Picture") );
      }

    }
    else
    {
      const KUrl url = contact.photo().url();
      if( !url.isEmpty() )
        setXfaceFromFile( url );
      else
        KMessageBox::information( this, i18n("No picture set for your address book entry."), i18n("No Picture") );
    }
  }

  void XFaceConfigurator::slotUpdateXFace()
  {
    QString str = mTextEdit->toPlainText();

    if ( !str.isEmpty() ) {
      if ( str.startsWith( QLatin1String("x-face:"), Qt::CaseInsensitive ) ) {
        str = str.remove( QLatin1String("x-face:"), Qt::CaseInsensitive );
        mTextEdit->setPlainText(str);
      }
      KXFace xf;
      const QPixmap p = QPixmap::fromImage( xf.toImage(str) );
      mXFaceLabel->setPixmap( p );
    } else {
      mXFaceLabel->clear();
    }
  }

} // namespace KMail

#include "xfaceconfigurator.moc"
