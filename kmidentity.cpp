// kmidentity.cpp

#include "kmidentity.h"
#include "kfileio.h"

#include <kconfig.h>
#include <kapplication.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kprocess.h>


//-----------------------------------------------------------------------------
QStringList KMIdentity::identities()
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver( config, "Identity" );

  QStringList result = config->readListEntry( "IdentityList" );
  if (!result.contains( i18n( "Default" ))) {
    result.remove( "unknown" );
    result.prepend( i18n( "Default" ));
  }
  return result;
}


//-----------------------------------------------------------------------------
void KMIdentity::saveIdentities( QStringList ids, bool aWithSync )
{
  KConfig* config = kapp->config();
  KConfigGroupSaver saver( config, "Identity" );

  if (ids.contains( i18n( "Default" )))
    ids.remove( i18n( "Default" ));
  config->writeEntry( "IdentityList", ids );

  if (aWithSync) config->sync();
}

//-----------------------------------------------------------------------------
QString KMIdentity::matchIdentity( const QString &addressList )
{
  QStringList ids = identities();
  KConfig* config = kapp->config();
  for(QStringList::ConstIterator it = ids.begin(); it != ids.end(); ++it)
  {
     const QString &id = (*it);
     KConfigGroupSaver saver( config, (id == i18n( "Default" )) ? 
		     QString("Identity") : "Identity-" + id );
     QString email = config->readEntry("Email Address");
     if (addressList.find(email, 0, false)!= -1)
        return id;
  }
  return QString::null;
}



//-----------------------------------------------------------------------------
KMIdentity::KMIdentity( QString id )
{
  mIdentity = id;
}


//-----------------------------------------------------------------------------
KMIdentity::~KMIdentity()
{
}


//-----------------------------------------------------------------------------
void KMIdentity::readConfig(void)
{
  KConfig* config = kapp->config();
  struct passwd* pw;
  char str[80];
  int i;

  KConfigGroupSaver saver( config, (mIdentity == i18n( "Default" )) ?
		     QString("Identity") : "Identity-" + mIdentity );

  mFullName = config->readEntry("Name");
  if (mFullName.isEmpty())
  {
    pw = getpwuid(getuid());
    if (pw)
    {
      mFullName = pw->pw_gecos;

      i = mFullName.find(',');
      if (i>0) mFullName.truncate(i);
    }
  }


  mEmailAddr = config->readEntry("Email Address");
  if (mEmailAddr.isEmpty())
  {
    pw = getpwuid(getuid());
    if (pw)
    {
      gethostname(str, 79);
      mEmailAddr = QString(pw->pw_name) + "@" + str;

    }
  }

  mVCardFile = config->readEntry("VCardFile");
  mOrganization = config->readEntry("Organization");
  mPgpIdentity = config->readEntry("Default PGP Key").local8Bit();
  mReplyToAddr = config->readEntry("Reply-To Address");
  mSignatureFile = config->readEntry("Signature File");
  mUseSignatureFile = config->readBoolEntry("UseSignatureFile", false);
  mSignatureInlineText = config->readEntry("Inline Signature");
  mFcc = config->readEntry("Fcc");
  mDrafts = config->readEntry("Drafts");
  if (mIdentity == i18n( "Default" ))
    mTransport = QString::null;
  else
    mTransport = config->readEntry("Transport");
}


//-----------------------------------------------------------------------------
void KMIdentity::writeConfig(bool aWithSync)
{
  KConfig* config = kapp->config();

  KConfigGroupSaver saver( config, (mIdentity == i18n( "Default" )) ?
		     QString("Identity") : "Identity-" + mIdentity );

  config->writeEntry("Identity", mIdentity);
  config->writeEntry("Name", mFullName);
  config->writeEntry("Organization", mOrganization);
  config->writeEntry("Default PGP Key", mPgpIdentity.data());
  config->writeEntry("Email Address", mEmailAddr);
  config->writeEntry("Reply-To Address", mReplyToAddr);
  config->writeEntry("Signature File", mSignatureFile);
  config->writeEntry("Inline Signature", mSignatureInlineText );
  config->writeEntry("UseSignatureFile", mUseSignatureFile );
  config->writeEntry("VCardFile", mVCardFile);
  config->writeEntry("Transport", mTransport);
  config->writeEntry("Fcc", mFcc);
  config->writeEntry("Drafts", mDrafts);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMIdentity::mailingAllowed(void) const
{
  return (!mFullName.isEmpty() && !mEmailAddr.isEmpty());
}


//-----------------------------------------------------------------------------
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
    mSignatureFile = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setSignatureInlineText(const QString &str )
{
    mSignatureInlineText = str;
}


//-----------------------------------------------------------------------------
void KMIdentity::setUseSignatureFile( bool flag )
{
  mUseSignatureFile = flag;
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
QString KMIdentity::signature(void) const
{
  QString result, sigcmd;

  if( mUseSignatureFile == false ) { return mSignatureInlineText; }

  if (mSignatureFile.isEmpty()) return QString::null;

  if (mSignatureFile.right(1)=="|")
  {
    KTempFile tmpf;
    int rc;

    tmpf.setAutoDelete(true);
    // signature file is a shell script that returns the signature
    if (tmpf.status() != 0) {
      QString wmsg = i18n("Failed to create temporary file\n%1:\n%2").arg(tmpf.name()).arg(strerror(errno));
      KMessageBox::information(0, wmsg);
      return QString::null;
    }
    tmpf.close();

    sigcmd = mSignatureFile.left(mSignatureFile.length()-1);
    sigcmd += " > ";
    sigcmd += tmpf.name();
    KShellProcess proc;
    proc << sigcmd;
    proc.start(KProcess::Block);
    rc = (proc.normalExit()) ? proc.exitStatus() : -1;

    if (rc != 0)
    {
      QString wmsg = i18n("Failed to execute signature script\n%1:\n%2").arg(sigcmd).arg(strerror(rc));
      KMessageBox::information(0, wmsg);
      return QString::null;
    }
    result = QString::fromLocal8Bit(kFileToString(tmpf.name(), TRUE, FALSE));
  }
  else
  {
    result = QString::fromLocal8Bit(kFileToString(mSignatureFile));
  }

  return result;
}
