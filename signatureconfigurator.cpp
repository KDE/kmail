/*  -*- c++ -*-
    Copyright 2008 Thomas McGuire <Thomas.McGuire@gmx.net>
    Copyright 2008 Edwin Schepers <yez@familieschepers.nl>
    Copyright 2004 Marc Mutz <mutz@kde.org>
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either 
    version 2.1 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public 
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "signatureconfigurator.h"

#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klineedit.h>
#include <kurlrequester.h>
#include <kshellcompletion.h>
#include <krun.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QMimeData>
#include <QTextEdit>

#include <QStackedWidget>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <assert.h>

using namespace KMail;

namespace KMail {

  SignatureConfigurator::SignatureConfigurator( QWidget * parent )
    : QWidget( parent )
  {
    // tmp. vars:
    QLabel * label;
    QWidget * page;
    QHBoxLayout * hlay;
    QVBoxLayout * vlay;
    QVBoxLayout * page_vlay;

    vlay = new QVBoxLayout( this );
    vlay->setObjectName( "main layout" );
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );

    // "enable signatue" checkbox:
    mEnableCheck = new QCheckBox( i18n("&Enable signature"), this );
    mEnableCheck->setWhatsThis(
        i18n("Check this box if you want KMail to append a signature to mails "
             "written with this identity."));
    vlay->addWidget( mEnableCheck );

    // "obtain signature text from" combo and label:
    hlay = new QHBoxLayout(); // inherits spacing
    vlay->addLayout( hlay );
    mSourceCombo = new QComboBox( this );
    mSourceCombo->setEditable( false );
    mSourceCombo->setWhatsThis(
        i18n("Click on the widgets below to obtain help on the input methods."));
    mSourceCombo->setEnabled( false ); // since !mEnableCheck->isChecked()
    mSourceCombo->addItems( QStringList()
                   << i18nc("continuation of \"obtain signature text from\"",
                           "Input Field Below")
                   << i18nc("continuation of \"obtain signature text from\"",
                           "File")
                   << i18nc("continuation of \"obtain signature text from\"",
                           "Output of Command")
                   );
    label = new QLabel( i18n("Obtain signature &text from:"), this );
    label->setBuddy( mSourceCombo );
    label->setEnabled( false ); // since !mEnableCheck->isChecked()
    hlay->addWidget( label );
    hlay->addWidget( mSourceCombo, 1 );

    // widget stack that is controlled by the source combo:
    QStackedWidget * widgetStack = new QStackedWidget( this );
    widgetStack->setEnabled( false ); // since !mEnableCheck->isChecked()
    vlay->addWidget( widgetStack, 1 );
    connect( mSourceCombo, SIGNAL(currentIndexChanged(int)),
             widgetStack, SLOT(setCurrentIndex (int)) );
    connect( mSourceCombo, SIGNAL(highlighted(int)),
             widgetStack, SLOT(setCurrentIndex (int)) );
    // connects for the enabling of the widgets depending on
    // signatureEnabled:
    connect( mEnableCheck, SIGNAL(toggled(bool)),
             mSourceCombo, SLOT(setEnabled(bool)) );
    connect( mEnableCheck, SIGNAL(toggled(bool)),
             widgetStack, SLOT(setEnabled(bool)) );
    connect( mEnableCheck, SIGNAL(toggled(bool)),
             label, SLOT(setEnabled(bool)) );
    // The focus might be still in the widget that is disabled
    connect( mEnableCheck, SIGNAL(clicked()),
             mEnableCheck, SLOT(setFocus()) );

    int pageno = 0;
    // page 0: input field for direct entering:
    page = new QWidget( widgetStack );
    widgetStack->insertWidget( pageno, page );
    page_vlay = new QVBoxLayout( page );
    mTextEdit = new QTextEdit();
    page_vlay->addWidget( mTextEdit, 0 );
    mTextEdit->setWhatsThis(
        i18n("Use this field to enter an arbitrary static signature."));
    mTextEdit->setFont( KGlobalSettings::fixedFont() );
    mTextEdit->setWordWrapMode( QTextOption::WrapAnywhere );

    hlay = new QHBoxLayout(); // inherits spacing
    page_vlay->addLayout( hlay );
    mHtmlCheck = new QCheckBox( i18n("&Use HTML"), page );
    connect( mHtmlCheck, SIGNAL(clicked()),
             this, SLOT(slotSetHtml()) );
    mShowCodeOrHtmlBtn = new QPushButton( i18n("Show HTML"), page );
    connect( mShowCodeOrHtmlBtn, SIGNAL(clicked()), SLOT(slotShowCodeOrHtml()) );
    hlay->addWidget( mHtmlCheck );
    hlay->addWidget( mShowCodeOrHtmlBtn, 1 );
    mInlinedHtml = true;

    widgetStack->setCurrentIndex( 0 ); // since mSourceCombo->currentItem() == 0

    // page 1: "signature file" requester, label, "edit file" button:
    ++pageno;
    page = new QWidget( widgetStack );
    widgetStack->insertWidget( pageno, page ); // force sequential numbers (play safe)
    page_vlay = new QVBoxLayout( page );
    page_vlay->setMargin( 0 );
    page_vlay->setSpacing( KDialog::spacingHint() );
    hlay = new QHBoxLayout(); // inherits spacing
    page_vlay->addLayout( hlay );
    mFileRequester = new KUrlRequester( page );
    mFileRequester->setWhatsThis(
        i18n("Use this requester to specify a text file that contains your "
             "signature. It will be read every time you create a new mail or "
             "append a new signature."));
    label = new QLabel( i18n("S&pecify file:"), page );
    label->setBuddy( mFileRequester );
    hlay->addWidget( label );
    hlay->addWidget( mFileRequester, 1 );
    mFileRequester->button()->setAutoDefault( false );
    connect( mFileRequester, SIGNAL(textChanged(const QString &)),
             this, SLOT(slotEnableEditButton(const QString &)) );
    mEditButton = new QPushButton( i18n("Edit &File"), page );
    mEditButton->setWhatsThis( i18n("Opens the specified file in a text editor."));
    connect( mEditButton, SIGNAL(clicked()), SLOT(slotEdit()) );
    mEditButton->setAutoDefault( false );
    mEditButton->setEnabled( false ); // initially nothing to edit
    hlay->addWidget( mEditButton );
    page_vlay->addStretch( 1 ); // spacer

    // page 2: "signature command" requester and label:
    ++pageno;
    page = new QWidget( widgetStack );
    widgetStack->insertWidget( pageno,page );
    page_vlay = new QVBoxLayout( page  );
    page_vlay->setMargin( 0 );
    page_vlay->setSpacing( KDialog::spacingHint() );
    hlay = new QHBoxLayout(); // inherits spacing
    page_vlay->addLayout( hlay );
    mCommandEdit = new KLineEdit( page );
    mCommandEdit->setCompletionObject( new KShellCompletion() );
    mCommandEdit->setAutoDeleteCompletionObject( true );
    mCommandEdit->setWhatsThis(
        i18n("You can add an arbitrary command here, either with or without path "
             "depending on whether or not the command is in your Path. For every "
             "new mail, KMail will execute the command and use what it outputs (to "
             "standard output) as a signature. Usual commands for use with this "
             "mechanism are \"fortune\" or \"ksig -random\"."));
    label = new QLabel( i18n("S&pecify command:"), page );
    label->setBuddy( mCommandEdit );
    hlay->addWidget( label );
    hlay->addWidget( mCommandEdit, 1 );
    page_vlay->addStretch( 1 ); // spacer
  }

  SignatureConfigurator::~SignatureConfigurator()
  {

  }

  bool SignatureConfigurator::isSignatureEnabled() const
  {
    return mEnableCheck->isChecked();
  }

  void SignatureConfigurator::setSignatureEnabled( bool enable )
  {
    mEnableCheck->setChecked( enable );
  }

  Signature::Type SignatureConfigurator::signatureType() const
  {
    if ( !isSignatureEnabled() ) return Signature::Disabled;

    switch ( mSourceCombo->currentIndex() ) {
    case 0:  return Signature::Inlined;
    case 1:  return Signature::FromFile;
    case 2:  return Signature::FromCommand;
    default: return Signature::Disabled;
    }
  }

  void SignatureConfigurator::setSignatureType( Signature::Type type )
  {
    setSignatureEnabled( type != Signature::Disabled );

    int idx = 0;
    switch( type ) {
    case Signature::Inlined:     idx = 0; break;
    case Signature::FromFile:    idx = 1; break;
    case Signature::FromCommand: idx = 2; break;
    default:                     idx = 0; break;
    };

    mSourceCombo->setCurrentIndex( idx );
  }

  QString SignatureConfigurator::inlineText() const
  {
    if ( mHtmlCheck->checkState() == Qt::Unchecked ) 
      return mTextEdit->toPlainText();

    if ( mViewMode == ShowHtml )
      return asCleanedHTML();
    else
      return mTextEdit->toPlainText();
  }

  void SignatureConfigurator::setInlineText( const QString & text )
  {
    mTextEdit->setText( text );
  }

  QString SignatureConfigurator::fileURL() const
  {
    QString file = mFileRequester->url().path();

    // Force the filename to be relative to ~ instead of $PWD depending
    // on the rest of the code (KRun::run in Edit and KFileItem on save)
    if ( !file.isEmpty() && QFileInfo( file ).isRelative() )
        file = QDir::home().absolutePath() + QDir::separator() + file;

    return file;
  }

  void SignatureConfigurator::setFileURL( const QString & url )
  {
    mFileRequester->setUrl( url );
  }

  QString SignatureConfigurator::commandURL() const
  {
    return mCommandEdit->text();
  }

  void SignatureConfigurator::setCommandURL( const QString & url )
  {
    mCommandEdit->setText( url );
  }


  Signature SignatureConfigurator::signature() const
  {
    Signature sig;
    sig.setType( signatureType() );
    sig.setInlinedHtml( mInlinedHtml );
    sig.setText( inlineText() );
    if ( signatureType() == Signature::FromCommand )
      sig.setUrl( commandURL(), true );
    if ( signatureType() == Signature::FromFile )
      sig.setUrl( fileURL(), false );
    return sig;
  }

  void SignatureConfigurator::setSignature( const Signature & sig )
  {
    setSignatureType( sig.type() );
    if ( sig.isInlinedHtml() )
    {
      mHtmlCheck->setCheckState( Qt::Checked );
      mInlinedHtml = true;
      initHtmlState();
    }
    else
    {
      mInlinedHtml = false;
      mShowCodeOrHtmlBtn->setEnabled( false );
      mTextEdit->setAcceptRichText( false );
    }
    setInlineText( sig.text() );

    if ( sig.type() == Signature::FromFile )
      setFileURL( sig.url() );
    else
      setFileURL( QString() );

    if ( sig.type() == Signature::FromCommand )
      setCommandURL( sig.url() );
    else
      setCommandURL( QString() );
  }

  void SignatureConfigurator::slotEnableEditButton( const QString & url )
  {
    mEditButton->setDisabled( url.trimmed().isEmpty() );
  }

  void SignatureConfigurator::slotEdit()
  {
    QString url = fileURL();
    // slotEnableEditButton should prevent this assert from being hit:
    assert( !url.isEmpty() );

    (void)KRun::runUrl( KUrl( url ), QString::fromLatin1("text/plain"), this );
  }

  QString SignatureConfigurator::asCleanedHTML() const
  {
    QString text = mTextEdit->toHtml();
    text.remove( "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n" );
    text.remove( "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n" );
    text.remove( "p, li { white-space: pre-wrap; }\n" );
    text.remove( "</style></head><body style=\" font-family:'Monospace'; font-size:10pt; font-weight:400; font-style:normal;\">\n" );
    text.remove( "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">" );
    text.remove( "</p></body></html>" );
    return text;
  }

  // button clicked
  void SignatureConfigurator::slotShowCodeOrHtml()
  {
    if ( mViewMode == ShowCode )
      toggleHtmlBtnState( ShowHtml );
    else
      toggleHtmlBtnState( ShowCode );
  }

  void SignatureConfigurator::initHtmlState()
  {
    mShowCodeOrHtmlBtn->setEnabled( true );
    mTextEdit->setAcceptRichText( true );
    mViewMode = ShowHtml;
    mShowCodeOrHtmlBtn->setText( i18n("Show HTML Code") );
  }

  void SignatureConfigurator::toggleHtmlBtnState( ViewMode state )
  {
    switch ( state )
    {
      case ShowCode:
        // show the html code, and toggle the button text
        mTextEdit->setAcceptRichText( false );
        mTextEdit->setPlainText( asCleanedHTML() );
        mViewMode = ShowCode;
        mShowCodeOrHtmlBtn->setText( i18n("Show HTML") );
        break;
      case ShowHtml:
        // show the formatted signature
        mTextEdit->setAcceptRichText( true );
        mViewMode = ShowHtml;
        mTextEdit->setHtml( mTextEdit->toPlainText() );
        mShowCodeOrHtmlBtn->setText( i18n("Show HTML Code") );
        break;
    }
  }

  // "use HTML"-checkbox (un)checked
  void SignatureConfigurator::slotSetHtml()
  {
    if ( mHtmlCheck->checkState() == Qt::Unchecked ) {
      mTextEdit->setAcceptRichText( false );
      mShowCodeOrHtmlBtn->setEnabled( false );
      mInlinedHtml = false;
      if ( mViewMode == ShowCode )
        mTextEdit->setHtml( mTextEdit->toPlainText() );
      mTextEdit->setPlainText( mTextEdit->toPlainText() );
    }
    else {
      mInlinedHtml = true;
      initHtmlState();
    }
  }

} // namespace KMail

#include "signatureconfigurator.moc"
