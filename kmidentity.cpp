// kmidentity.cpp

#include "kmidentity.h"
#include "kfileio.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmkernel.h"

#include <kdebug.h>
#include <kapplication.h>
#include <kurl.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kconfig.h>

#include <qstringlist.h>
#include <qfileinfo.h>
#include <qdatastream.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

Signature::Signature()
  : mType( Disabled )
{

}

Signature::Signature( const QString & text )
  : mText( text ),
    mType( Inlined )
{

}

Signature::Signature( const QString & url, bool isExecutable )
  : mUrl( url ),
    mType( isExecutable ? FromCommand : FromFile )
{
}

bool Signature::operator==( const Signature & other ) const {
  if ( mType != other.mType ) return false;
  switch ( mType ) {
  case Inlined: return mText == other.mText;
  case FromFile:
  case FromCommand: return mUrl == other.mUrl;
  default:
  case Disabled: return true;
  }
}

QString Signature::rawText( bool * ok ) const
{
  switch ( mType ) {
  case Disabled:
    if ( ok ) *ok = true;
    return QString::null;
  case Inlined:
    if ( ok ) *ok = true;
    return mText;
  case FromFile:
    return textFromFile( ok );
  case FromCommand:
    return textFromCommand( ok );
  };
  kdFatal( 5006 ) << "Signature::type() returned unknown value!" << endl;
  return QString::null; // make compiler happy
}

QString Signature::textFromCommand( bool * ok ) const
{
  assert( mType == FromCommand );

  // handle pathological cases:
  if ( mUrl.isEmpty() ) {
    if ( ok ) *ok = true;
    return QString::null;
  }

  // create a shell process:
  KProcess proc;
  proc.setUseShell(true);
  proc << mUrl;

  // let the kernel collect the output for us:
  QObject::connect( &proc, SIGNAL(receivedStdout(KProcess*,char*,int)),
		    kernel, SLOT(slotCollectStdOut(KProcess*,char*,int)) );

  // run the process:
  int rc = 0;
  if ( !proc.start( KProcess::Block, KProcess::Stdout ) )
    rc = -1;
  else
    rc = ( proc.normalExit() ) ? proc.exitStatus() : -1 ;

  // get output:
  QByteArray output = kernel->getCollectedStdOut( &proc );

  // handle errors, if any:
  if ( rc != 0 ) {
    if ( ok ) *ok = false;
    QString wmsg = i18n("<qt>Failed to execute signature script<br><b>%1</b>:<br>%2</qt>")
      .arg( mUrl ).arg( strerror(rc) );
    KMessageBox::error(0, wmsg);
    return QString::null;
  }

  // no errors:
  if ( ok ) *ok = true;
  // ### hmm, should we allow other encodings, too?
  return QString::fromLocal8Bit( output.data(), output.size() );
}

QString Signature::textFromFile( bool * ok ) const
{
  assert( mType == FromFile );

  // ### FIXME: Use KIO::NetAccess to download non-local files!
  if ( !KURL(mUrl).isLocalFile() && !(QFileInfo(mUrl).isRelative()
					&& QFileInfo(mUrl).exists()) ) {
    kdDebug( 5006 ) << "Signature::textFromFile: non-local URLs are unsupported" << endl;
    if ( ok ) *ok = false;
    return QString::null;
  }
  if ( ok ) *ok = true;
  // ### hmm, should we allow other encodings, too?
  return QString::fromLocal8Bit( kFileToString( mUrl ) );
}

QString Signature::withSeparator( bool * ok ) const
{
  bool internalOK = false;
  QString signature = rawText( &internalOK );
  if ( !internalOK ) {
    if ( ok ) *ok = false;
    return QString::null;
  }
  if ( ok ) *ok = true;
  if ( signature.isEmpty() ) return signature; // don't add a separator in this case
  if ( signature.startsWith( QString::fromLatin1("-- \n") ) ||
       signature.find( QString::fromLatin1("\n-- \n") ) != -1 )
    // already have signature separator:
    return signature;
  else
    // need to prepend one:
    return QString::fromLatin1("-- \n") + signature;
}


void Signature::setUrl( const QString & url, bool isExecutable )
{
  mUrl = url;
  mType = isExecutable ? FromCommand : FromFile ;
}

// config keys and values:
static const char * sigTypeKey = "Signature Type";
static const char * sigTypeInlineValue = "inline";
static const char * sigTypeFileValue = "file";
static const char * sigTypeCommandValue = "command";
static const char * sigTypeDisabledValue = "disabled";
static const char * sigTextKey = "Inline Signature";
static const char * sigFileKey = "Signature File";
static const char * sigCommandKey = "Signature Command";

void Signature::readConfig( const KConfigBase * config )
{
  QString sigType = config->readEntry( sigTypeKey );
  if ( sigType == sigTypeInlineValue ) {
    mType = Inlined;
    mText = config->readEntry( sigTextKey );
  } else if ( sigType == sigTypeFileValue ) {
    mType = FromFile;
    mUrl = config->readEntry( sigFileKey );
  } else if ( sigType == sigTypeCommandValue ) {
    mType = FromCommand;
    mUrl = config->readEntry( sigCommandKey );
  } else {
    mType = Disabled;
  }
}

void Signature::writeConfig( KConfigBase * config ) const
{
  switch ( mType ) {
  case Inlined:
    config->writeEntry( sigTypeKey, sigTypeInlineValue );
    config->writeEntry( sigTextKey, mText );
    break;
  case FromFile:
    config->writeEntry( sigTypeKey, sigTypeFileValue );
    config->writeEntry( sigFileKey, mUrl );
    break;
  case FromCommand:
    config->writeEntry( sigTypeKey, sigTypeCommandValue );
    config->writeEntry( sigCommandKey, mUrl );
    break;
  case Disabled:
    config->writeEntry( sigTypeKey, sigTypeDisabledValue );
  default: ;
  }
}

QDataStream & operator<<( QDataStream & stream, const Signature & sig ) {
  return stream << static_cast<Q_UINT8>(sig.mType)
		<< sig.mUrl
		<< sig.mText;
}

QDataStream & operator>>( QDataStream & stream, Signature & sig ) {
    Q_UINT8 s;
    stream >> s
           >> sig.mUrl
           >> sig.mText;
    sig.mType = static_cast<Signature::Type>(s);
    return stream;
}

KMIdentity KMIdentity::null;

bool KMIdentity::isNull() const {
  return mIdentity.isNull() && mFullName.isNull() && mEmailAddr.isNull() &&
    mOrganization.isNull() && mReplyToAddr.isNull() && mBcc.isNull() &&
    mVCardFile.isNull() &&
    mPgpIdentity.isNull() && mFcc.isNull() && mDrafts.isNull() &&
    mTransport.isNull() && mSignature.type() == Signature::Disabled;
}

bool KMIdentity::operator==( const KMIdentity & other ) const {
    return mUoid == other.mUoid &&
      mIdentity == other.mIdentity && mFullName == other.mFullName &&
      mEmailAddr == other.mEmailAddr && mOrganization == other.mOrganization &&
      mReplyToAddr == other.mReplyToAddr && mBcc == other.mBcc &&
      mVCardFile == other.mVCardFile &&
      mPgpIdentity == other.mPgpIdentity && mFcc == other.mFcc &&
      mDrafts == other.mDrafts && mTransport == other.mTransport &&
      mSignature == other.mSignature;
}

KMIdentity::KMIdentity( const QString & id, const QString & fullName,
			const QString & emailAddr, const QString & organization,
			const QString & replyToAddr )
  : mUoid( 0 ), mIdentity( id ), mFullName( fullName ),
    mEmailAddr( emailAddr ), mOrganization( organization ),
    mReplyToAddr( replyToAddr ), mIsDefault( false )
{

}

KMIdentity::~KMIdentity()
{
}


void KMIdentity::readConfig( const KConfigBase * config )
{
  mUoid = config->readUnsignedNumEntry("uoid",0);

  mIdentity = config->readEntry("Identity");
  mFullName = config->readEntry("Name");
  mEmailAddr = config->readEntry("Email Address");
  mVCardFile = config->readEntry("VCardFile");
  mOrganization = config->readEntry("Organization");
  mPgpIdentity = config->readEntry("Default PGP Key").local8Bit();
  mReplyToAddr = config->readEntry("Reply-To Address");
  mBcc = config->readEntry("Bcc");
  mFcc = config->readEntry("Fcc", "sent-mail");
  if( mFcc.isEmpty() )
    mFcc = "sent-mail";
  mDrafts = config->readEntry("Drafts", "drafts");
  if( mDrafts.isEmpty() )
    mDrafts = "drafts";
  mTransport = config->readEntry("Transport");

  mSignature.readConfig( config );
  kdDebug(5006) << "KMIdentity::readConfig(): UOID = " << mUoid
	    << " for identity named \"" << mIdentity << "\"" << endl;
}


void KMIdentity::writeConfig( KConfigBase * config ) const
{
  config->writeEntry("uoid", mUoid);

  config->writeEntry("Identity", mIdentity);
  config->writeEntry("Name", mFullName);
  config->writeEntry("Organization", mOrganization);
  config->writeEntry("Default PGP Key", mPgpIdentity.data());
  config->writeEntry("Email Address", mEmailAddr);
  config->writeEntry("Reply-To Address", mReplyToAddr);
  config->writeEntry("Bcc", mBcc);
  config->writeEntry("VCardFile", mVCardFile);
  config->writeEntry("Transport", mTransport);
  config->writeEntry("Fcc", mFcc);
  config->writeEntry("Drafts", mDrafts);

  mSignature.writeConfig( config );
}

QDataStream & operator<<( QDataStream & stream, const KMIdentity & i ) {
  return stream << static_cast<Q_UINT32>(i.uoid())
		<< i.identityName()
		<< i.fullName()
		<< i.organization()
		<< i.pgpIdentity()
		<< i.emailAddr()
		<< i.replyToAddr()
		<< i.bcc()
		<< i.vCardFile()
		<< i.transport()
		<< i.fcc()
		<< i.drafts()
		<< i.mSignature;
}

QDataStream & operator>>( QDataStream & stream, KMIdentity & i ) {
#if 1 // why doesn't it work like for Q_UINT8 above???
  Q_UINT32 uoid;
  stream >> uoid;
  i.mUoid = uoid;
  return stream
#else
  return stream >> (Q_UINT32)i.mUoid
#endif
		>> i.mIdentity
		>> i.mFullName
		>> i.mOrganization
		>> i.mPgpIdentity
		>> i.mEmailAddr
		>> i.mReplyToAddr
		>> i.mBcc
		>> i.mVCardFile
		>> i.mTransport
		>> i.mFcc
		>> i.mDrafts
		>> i.mSignature;
}

//-----------------------------------------------------------------------------
bool KMIdentity::mailingAllowed() const
{
  return !mEmailAddr.isEmpty();
}


void KMIdentity::setIsDefault( bool flag ) {
  mIsDefault = flag;
}

void KMIdentity::setIdentityName( const QString & name ) {
  mIdentity = name;
}

void KMIdentity::setFullName(const QString &str)
{
  mFullName = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setOrganization(const QString &str)
{
  mOrganization = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setPgpIdentity(const QCString &str)
{
  mPgpIdentity = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setEmailAddr(const QString &str)
{
  mEmailAddr = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setVCardFile(const QString &str)
{
  mVCardFile = str;
}


//-----------------------------------------------------------------------------
QString KMIdentity::fullEmailAddr(void) const
{
  if (mFullName.isEmpty()) return mEmailAddr;

  const QString specials("()<>@,.;:[]");

  QString result;

  // add DQUOTE's if necessary:
  bool needsQuotes=false;
  for (unsigned int i=0; i < mFullName.length(); i++) {
    if ( specials.contains( mFullName[i] ) )
      needsQuotes = true;
    else if ( mFullName[i] == '\\' || mFullName[i] == '"' ) {
      needsQuotes = true;
      result += '\\';
    }
    result += mFullName[i];
  }

  if (needsQuotes) {
    result.insert(0,'"');
    result += '"';
  }

  result += " <" + mEmailAddr + '>';

  return result;
}

//-----------------------------------------------------------------------------
void KMIdentity::setReplyToAddr(const QString& str)
{
  mReplyToAddr = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setSignatureFile(const QString &str)
{
  mSignature.setUrl( str, signatureIsCommand() );
}


//-----------------------------------------------------------------------------
void KMIdentity::setSignatureInlineText(const QString &str )
{
  mSignature.setText( str );
}


//-----------------------------------------------------------------------------
void KMIdentity::setTransport(const QString &str)
{
  mTransport = str;
}

//-----------------------------------------------------------------------------
void KMIdentity::setFcc(const QString &str)
{
  mFcc = str;
}

//-----------------------------------------------------------------------------
void KMIdentity::setDrafts(const QString &str)
{
  mDrafts = str;
}


//-----------------------------------------------------------------------------
QString KMIdentity::signatureText( bool * ok ) const
{
  bool internalOK = false;
  QString signatureText = mSignature.withSeparator( &internalOK );
  if ( internalOK ) {
    if ( ok ) *ok=true;
    return signatureText;
  }

  // OK, here comes the funny part. The call to
  // Signature::withSeparator() failed, so we should probably fix the
  // cause:
  if ( ok ) *ok = false;
  return QString::null;

#if 0 // ### FIXME: error handling
  if (mSignatureFile.endsWith("|"))
  {
  }
  else
  {
  }
#endif

  return QString::null;
}
