// kmidentity.cpp

#include "kmidentity.h"
#include "kfileio.h"

#include <kconfig.h>
#include <kapp.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kmessagebox.h>


//-----------------------------------------------------------------------------
KMIdentity::KMIdentity()
{
  readConfig();
}


//-----------------------------------------------------------------------------
KMIdentity::~KMIdentity()
{
  writeConfig();
}


//-----------------------------------------------------------------------------
void KMIdentity::readConfig(void)
{
  KConfig* config = kapp->config();
  struct passwd* pw;
  char str[80];
  int i;

  config->setGroup("Identity");

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

  mOrganization = config->readEntry("Organization");
  mReplyToAddr = config->readEntry("Reply-To Address");
  mSignatureFile = config->readEntry("Signature File");
}


//-----------------------------------------------------------------------------
void KMIdentity::writeConfig(bool aWithSync)
{
  KConfig* config = kapp->config();
  config->setGroup("Identity");

  config->writeEntry("Name", mFullName);
  config->writeEntry("Organization", mOrganization);
  config->writeEntry("Email Address", mEmailAddr);
  config->writeEntry("Reply-To Address", mReplyToAddr);
  config->writeEntry("Signature File", mSignatureFile);

  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
bool KMIdentity::mailingAllowed(void) const
{
  return (!mFullName.isEmpty() && !mEmailAddr.isEmpty());
}


//-----------------------------------------------------------------------------
void KMIdentity::setFullName(const QString str)
{
  mFullName = str.copy();
}


//-----------------------------------------------------------------------------
void KMIdentity::setOrganization(const QString str)
{
  mOrganization = str.copy();
}


//-----------------------------------------------------------------------------
void KMIdentity::setEmailAddr(const QString str)
{
  mEmailAddr = str.copy();
}


//-----------------------------------------------------------------------------
const QString KMIdentity::fullEmailAddr(void) const
{
  QString result;

  if (mFullName.isEmpty()) result = mEmailAddr.copy();
  else result = mFullName.copy() + " <" + mEmailAddr + ">";

  return result;
}

//-----------------------------------------------------------------------------
void KMIdentity::setReplyToAddr(const QString str)
{
  mReplyToAddr = str.copy();
}


//-----------------------------------------------------------------------------
void KMIdentity::setSignatureFile(const QString str)
{
  mSignatureFile = str.copy();
}


//-----------------------------------------------------------------------------
const QString KMIdentity::signature(void) const
{
  QString result, sigcmd;

  if (mSignatureFile.isEmpty()) return QString::null;

  if (mSignatureFile.right(1)=="|")
  {
    KTempFile tmpf;
    int rc;

    tmpf.setAutoDelete(true);
    // signature file is a shell script that returns the signature
    if (tmpf.status() != 0) {
      QString wmsg = QString("%1\n%2").arg(tmpf.name()).arg(strerror(errno));
      KMessageBox::information(0, i18n("Failed to create temporary file\n") +
			       wmsg);
      return QString::null;
    }
    tmpf.close();

    sigcmd = mSignatureFile.left(mSignatureFile.length()-1);
    sigcmd += " >";
    sigcmd += tmpf.name();
    rc = system(sigcmd);

    if (rc != 0)
    {
      QString wmsg = QString("%1\n%2").arg(sigcmd.data()).arg(strerror(errno));
      KMessageBox::information(0, i18n("Failed to execute signature script\n") +
			       wmsg );
      return QString::null;
    }
    result = kFileToString(tmpf.name(), TRUE, FALSE);
  }
  else
  {
    result = kFileToString(mSignatureFile);
  }

  return result;
}
