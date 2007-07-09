// kmmsgpartdlg.cpp


// my includes:
#include "kmmsgpartdlg.h"

// other KMail includes:
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kcursorsaver.h"

// other kdenetwork includes: (none)

// other KDE includes:
#include <kmimetype.h>
#include <kiconloader.h>
#include <kaboutdata.h>
#include <kstringvalidator.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcomponentdata.h>

// other Qt includes:
#include <QLabel>
#include <QLayout>

//Added by qt3to4:
#include <QGridLayout>
#include <QByteArray>
#include <klineedit.h>
#include <QCheckBox>

// other includes:
#include <assert.h>

static const struct {
  KMMsgPartDialog::Encoding encoding;
  const char * displayName;
} encodingTypes[] = {
  { KMMsgPartDialog::SevenBit, I18N_NOOP("None (7-bit text)") },
  { KMMsgPartDialog::EightBit, I18N_NOOP("None (8-bit text)") },
  { KMMsgPartDialog::QuotedPrintable, I18N_NOOP("Quoted Printable") },
  { KMMsgPartDialog::Base64, I18N_NOOP("Base 64") },
};
static const int numEncodingTypes =
  sizeof encodingTypes / sizeof *encodingTypes;

KMMsgPartDialog::KMMsgPartDialog( const QString & caption,
                                  QWidget * parent )
  : KDialog( parent )
{
  setCaption( caption.isEmpty() ? i18n("Message Part Properties") : caption );
  setButtons( Ok|Cancel|Help );
  setDefaultButton( Ok );
  setModal( true );
  showButtonSeparator( true );

  // tmp vars:
  QGridLayout * glay;
  QLabel      * label;
  QString       msg;

  setHelp( QString::fromLatin1("attachments") );

  for ( int i = 0 ; i < numEncodingTypes ; ++i )
    mI18nizedEncodings << i18n( encodingTypes[i].displayName );
  QFrame *frame = new QFrame( this );
  setMainWidget( frame );
  glay = new QGridLayout(frame );
  glay->setSpacing( spacingHint() );
  glay->setColumnStretch( 1, 1 );
  glay->setRowStretch( 8, 1 );

  // mimetype icon:
  mIcon = new QLabel( frame );
  mIcon->setPixmap( DesktopIcon("unknown") );
  glay->addWidget( mIcon, 0, 0, 2, 1);

  // row 0: Type combobox:
  mMimeType = new KComboBox( true, frame );
  mMimeType->setInsertPolicy( QComboBox::NoInsert );
  mMimeType->setValidator( new KMimeTypeValidator( mMimeType ) );
  mMimeType->addItems( QStringList()
                               << QString::fromLatin1("text/html")
                               << QString::fromLatin1("text/plain")
                               << QString::fromLatin1("image/gif")
                               << QString::fromLatin1("image/jpeg")
                               << QString::fromLatin1("image/png")
                               << QString::fromLatin1("application/octet-stream")
                               << QString::fromLatin1("application/x-gunzip")
                               << QString::fromLatin1("application/zip") );
  connect( mMimeType, SIGNAL(textChanged(const QString&)),
           this, SLOT(slotMimeTypeChanged(const QString&)) );
  glay->addWidget( mMimeType, 0, 1 );

  msg = i18n("<qt><p>The <em>MIME type</em> of the file:</p>"
             "<p>normally, you do not need to touch this setting, since the "
             "type of the file is automatically checked; but, sometimes, %1 "
             "may not detect the type correctly -- here is where you can fix "
             "that.</p></qt>", KGlobal::mainComponent().aboutData()->programName() );
  mMimeType->setWhatsThis( msg );

  // row 1: Size label:
  mSize = new QLabel( frame );
  setSize( KIO::filesize_t(0) );
  glay->addWidget( mSize, 1, 1 );

  msg = i18n("<qt><p>The size of the part:</p>"
             "<p>sometimes, %1 will only give an estimated size here, "
             "because calculating the exact size would take too much time; "
             "when this is the case, it will be made visible by adding "
             "\"(est.)\" to the size displayed.</p></qt>",
      KGlobal::mainComponent().aboutData()->programName() );
  mSize->setWhatsThis( msg );

  // row 2: "Name" lineedit and label:
  mFileName = new KLineEdit( frame );
  label = new QLabel( i18n("&Name:"), frame );
  label->setBuddy( mFileName );
  glay->addWidget( label, 2, 0 );
  glay->addWidget( mFileName, 2, 1 );

  msg = i18n("<qt><p>The file name of the part:</p>"
             "<p>although this defaults to the name of the attached file, "
             "it does not specify the file to be attached; rather, it "
             "suggests a file name to be used by the recipient's mail agent "
             "when saving the part to disk.</p></qt>");
  label->setWhatsThis( msg );
  mFileName->setWhatsThis( msg );

  // row 3: "Description" lineedit and label:
  mDescription = new KLineEdit( frame );
  label = new QLabel( i18n("&Description:"), frame );
  label->setBuddy( mDescription );
  glay->addWidget( label, 3, 0 );
  glay->addWidget( mDescription, 3, 1 );

  msg = i18n("<qt><p>A description of the part:</p>"
             "<p>this is just an informational description of the part, "
             "much like the Subject is for the whole message; most "
             "mail agents will show this information in their message "
             "previews alongside the attachment's icon.</p></qt>");
  label->setWhatsThis( msg );
  mDescription->setWhatsThis( msg );

  // row 4: "Encoding" combobox and label:
  mEncoding = new QComboBox( frame );
  mEncoding->setEditable( false );
  mEncoding->addItems( mI18nizedEncodings );
  label = new QLabel( i18n("&Encoding:"), frame );
  label->setBuddy( mEncoding );
  glay->addWidget( label, 4, 0 );
  glay->addWidget( mEncoding, 4, 1 );

  msg = i18n("<qt><p>The transport encoding of this part:</p>"
             "<p>normally, you do not need to change this, since %1 will use "
             "a decent default encoding, depending on the MIME type; yet, "
             "sometimes, you can significantly reduce the size of the "
             "resulting message, e.g. if a PostScript file does not contain "
             "binary data, but consists of pure text -- in this case, choosing "
             "\"quoted-printable\" over the default \"base64\" will save up "
             "to 25% in resulting message size.</p></qt>",
      KGlobal::mainComponent().aboutData()->programName() );
  label->setWhatsThis( msg );
  mEncoding->setWhatsThis( msg );

  // row 5: "Suggest automatic display..." checkbox:
  mInline = new QCheckBox( i18n("Suggest &automatic display"), frame );
  glay->addWidget( mInline, 5, 0, 1, 2 );

  msg = i18n("<qt><p>Check this option if you want to suggest to the "
             "recipient the automatic (inline) display of this part in the "
             "message preview, instead of the default icon view;</p>"
             "<p>technically, this is carried out by setting this part's "
             "<em>Content-Disposition</em> header field to \"inline\" "
             "instead of the default \"attachment\".</p></qt>");
  mInline->setWhatsThis( msg );

  // row 6: "Sign" checkbox:
  mSigned = new QCheckBox( i18n("&Sign this part"), frame );
  glay->addWidget( mSigned, 6, 0, 1, 2 );

  msg = i18n("<qt><p>Check this option if you want this message part to be "
             "signed;</p>"
             "<p>the signature will be made with the key that you associated "
             "with the currently-selected identity.</p></qt>");
  mSigned->setWhatsThis( msg );

  // row 7: "Encrypt" checkbox:
  mEncrypted = new QCheckBox( i18n("Encr&ypt this part"), frame );
  glay->addWidget( mEncrypted, 7, 0, 1, 2 );

  msg = i18n("<qt><p>Check this option if you want this message part to be "
             "encrypted;</p>"
             "<p>the part will be encrypted for the recipients of this "
             "message</p></qt>");
  mEncrypted->setWhatsThis( msg );
  // (row 8: spacer)
}


KMMsgPartDialog::~KMMsgPartDialog() {}


QString KMMsgPartDialog::mimeType() const {
  return mMimeType->currentText();
}

void KMMsgPartDialog::setMimeType( const QString & mimeType ) {
  for ( int i = 0 ; i < mMimeType->count() ; ++i )
    if ( mMimeType->itemText( i ) == mimeType ) {
      mMimeType->setCurrentIndex( i );
      slotMimeTypeChanged( mimeType );
      return;
    }
  mMimeType->insertItem( 0, mimeType );
  mMimeType->setCurrentIndex( 0 );
  slotMimeTypeChanged( mimeType );
}

void KMMsgPartDialog::setMimeType( const QString & type,
                                   const QString & subtype ) {
  setMimeType( QString::fromLatin1("%1/%2").arg(type).arg(subtype) );
}

void KMMsgPartDialog::setMimeTypeList( const QStringList & mimeTypes ) {
  mMimeType->addItems( mimeTypes );
}

void KMMsgPartDialog::setSize( KIO::filesize_t size, bool estimated ) {
  QString text = KIO::convertSize( size );
  if ( estimated )
    text = i18nc("%1: a filesize incl. unit (e.g. \"1.3 KB\")",
                "%1 (est.)", text );
  mSize->setText( text );
}

QString KMMsgPartDialog::fileName() const {
  return mFileName->text();
}

void KMMsgPartDialog::setFileName( const QString & fileName ) {
  mFileName->setText( fileName );
}

QString KMMsgPartDialog::description() const {
  return mDescription->text();
}

void KMMsgPartDialog::setDescription( const QString & description ) {
  mDescription->setText( description );
}

KMMsgPartDialog::Encoding KMMsgPartDialog::encoding() const {
  QString s = mEncoding->currentText();
  for ( int i = 0 ; i < mI18nizedEncodings.count() ; ++i )
    if ( s == mI18nizedEncodings.at(i) )
      return encodingTypes[i].encoding;
  kFatal(5006) << "KMMsgPartDialog::encoding(): Unknown encoding encountered!"
                << endl;
  return None; // keep compiler happy
}

void KMMsgPartDialog::setEncoding( Encoding encoding ) {
  for ( int i = 0 ; i < numEncodingTypes ; ++i )
    if ( encodingTypes[i].encoding == encoding ) {
      QString text = mI18nizedEncodings.at(i);
      for ( int j = 0 ; j < mEncoding->count() ; ++j )
        if ( mEncoding->itemText(j) == text ) {
          mEncoding->setCurrentIndex( j );
          return;
        }
      mEncoding->insertItem( 0, text );
      mEncoding->setCurrentIndex( 0 );
    }
  kFatal(5006) << "KMMsgPartDialog::setEncoding(): "
    "Unknown encoding encountered!" << endl;
}

void KMMsgPartDialog::setShownEncodings( int encodings ) {
  mEncoding->clear();
  for ( int i = 0 ; i < numEncodingTypes ; ++i )
    if ( encodingTypes[i].encoding & encodings )
      mEncoding->addItem( mI18nizedEncodings.at(i) );
}

bool KMMsgPartDialog::isInline() const {
  return mInline->isChecked();
}

void KMMsgPartDialog::setInline( bool inlined ) {
  mInline->setChecked( inlined );
}

bool KMMsgPartDialog::isEncrypted() const {
  return mEncrypted->isChecked();
}

void KMMsgPartDialog::setEncrypted( bool encrypted ) {
  mEncrypted->setChecked( encrypted );
}

void KMMsgPartDialog::setCanEncrypt( bool enable ) {
  mEncrypted->setEnabled( enable );
}

bool KMMsgPartDialog::isSigned() const {
  return mSigned->isChecked();
}

void KMMsgPartDialog::setSigned( bool sign ) {
  mSigned->setChecked( sign );
}

void KMMsgPartDialog::setCanSign( bool enable ) {
  mSigned->setEnabled( enable );
}

void KMMsgPartDialog::slotMimeTypeChanged( const QString & mimeType ) {
  // message subparts MUST have 7bit or 8bit encoding...
#if 0
  // ...but until KMail can recode 8bit messages on attach, so that
  // they can be signed, we can't enforce this :-(
  if ( mimeType.startsWith("message/") ) {
    setEncoding( SevenBit );
    mEncoding->setEnabled( false );
  } else {
    mEncoding->setEnabled( !mReadOnly );
  }
#endif
  // find a mimetype icon:
  KMimeType::Ptr mt = KMimeType::mimeType( mimeType );
  if ( !mt.isNull() )
    mIcon->setPixmap( KIconLoader::global()->loadMimeTypeIcon( mt->iconName(),
                      K3Icon::Desktop ) );
  else 
    mIcon->setPixmap( DesktopIcon("unknown") );
}




KMMsgPartDialogCompat::KMMsgPartDialogCompat( const char *, bool readOnly)
  : KMMsgPartDialog(), mMsgPart( 0 )
{
  setShownEncodings( SevenBit|EightBit|QuotedPrintable|Base64 );
  if (readOnly)
  {
    mMimeType->setEditable(false);
    mMimeType->setEnabled(false);
    mFileName->setReadOnly(true);
    mDescription->setReadOnly(true);
    mEncoding->setEnabled(false);
    mInline->setEnabled(false);
    mEncrypted->setEnabled(false);
    mSigned->setEnabled(false);
  }
  connect(this,SIGNAL(okClicked()),SLOT(slotOk()));
}

KMMsgPartDialogCompat::~KMMsgPartDialogCompat() {}

void KMMsgPartDialogCompat::setMsgPart( KMMessagePart * aMsgPart )
{
  mMsgPart = aMsgPart;
  assert( mMsgPart );

  QByteArray enc = mMsgPart->cteStr();
  if ( enc == "7bit" )
    setEncoding( SevenBit );
  else if ( enc == "8bit" )
    setEncoding( EightBit );
  else if ( enc == "quoted-printable" )
    setEncoding( QuotedPrintable );
  else
    setEncoding( Base64 );

  setDescription( mMsgPart->contentDescription() );
  setFileName( mMsgPart->fileName() );
  setMimeType( mMsgPart->typeStr(), mMsgPart->subtypeStr() );
  setSize( mMsgPart->decodedSize() );
  QString cd(mMsgPart->contentDisposition());
  setInline( cd.indexOf( QRegExp("^\\s*inline", Qt::CaseInsensitive) ) >= 0 );
}


void KMMsgPartDialogCompat::applyChanges()
{
  if (!mMsgPart) return;

  KCursorSaver busy(KBusyPtr::busy());

  // apply Content-Disposition:
  QByteArray cDisp;
  if ( isInline() )
    cDisp = "inline;";
  else
    cDisp = "attachment;";

  QString name = fileName();
  if ( !name.isEmpty() || !mMsgPart->name().isEmpty()) {
    mMsgPart->setName( name );
    QByteArray encoding = KMMsgBase::autoDetectCharset( mMsgPart->charset(),
      KMMessage::preferredCharsets(), name );
    if ( encoding.isEmpty() ) encoding = "utf-8";
    QByteArray encName = KMMsgBase::encodeRFC2231String( name, encoding );

    cDisp += "\n\tfilename";
    if ( name != QString( encName ) )
      cDisp += "*=" + encName;
    else
      cDisp += "=\"" + encName.replace( '\\', "\\\\" ).replace( '"', "\\\"" ) + '"';
    mMsgPart->setContentDisposition( cDisp );
  }

  // apply Content-Description"
  QString desc = description();
  if ( !desc.isEmpty() || !mMsgPart->contentDescription().isEmpty() )
    mMsgPart->setContentDescription( desc );

  // apply Content-Type:
  QByteArray type = mimeType().toLatin1();
  QByteArray subtype;
  int idx = type.indexOf('/');
  if ( idx < 0 )
    subtype = "";
  else {
    subtype = type.mid( idx+1 );
    type = type.left( idx );
  }
  mMsgPart->setTypeStr(type);
  mMsgPart->setSubtypeStr(subtype);

  // apply Content-Transfer-Encoding:
  QByteArray cte;
  if (subtype == "rfc822" && type == "message")
    kWarning( encoding() != SevenBit && encoding() != EightBit, 5006 )
      << "encoding on rfc822/message must be \"7bit\" or \"8bit\"" << endl;
  switch ( encoding() ) {
  case SevenBit:        cte = "7bit";             break;
  case EightBit:        cte = "8bit";             break;
  case QuotedPrintable: cte = "quoted-printable"; break;
  case Base64: default: cte = "base64";           break;
  }
  if ( cte != mMsgPart->cteStr().toLower() ) {
    QByteArray body = mMsgPart->bodyDecodedBinary();
    mMsgPart->setCteStr( cte );
    mMsgPart->setBodyEncodedBinary( body );
  }
}


//-----------------------------------------------------------------------------
void KMMsgPartDialogCompat::slotOk()
{
  applyChanges();
}


//-----------------------------------------------------------------------------
#include "kmmsgpartdlg.moc"
