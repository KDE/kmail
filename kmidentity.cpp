// kmidentity.cpp

#include "kmidentity.h"

#include <kapp.h>
#include <kconfig.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/utsname.h>


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
  KConfig* config = kapp->getConfig();
  struct passwd* pw;
  char str[80];

  config->setGroup("Identity");

  mFullName = config->readEntry("Name");
  if (mFullName.isEmpty())
  {
    pw = getpwuid(getuid());
    if (pw)
    {
      mFullName = pw->pw_gecos;
      mFullName.detach();
    }
  }


  mEmailAddr = config->readEntry("Email Address");
  if (mEmailAddr.isEmpty())
  {
    pw = getpwuid(getuid());
    if (pw)
    {
      struct utsname uts;
      if (uname(&uts)==0)
      {
	strcpy(str, uts.nodename);
	if (uts.domainname[0] && strcmp(uts.domainname,"(none)")!=0)
	{
	  strcat(str, ".");
	  strcat(str, uts.domainname);
	}
      }
      else strcpy(str,"localhost");

      mEmailAddr = QString(pw->pw_name) + "@" + str;
      mEmailAddr.detach();
    }
  }

  mOrganization = config->readEntry("Organization");
  mReplyToAddr = config->readEntry("Reply-To Address");
  mSignatureFile = config->readEntry("Signature File");
}


//-----------------------------------------------------------------------------
void KMIdentity::writeConfig(bool aWithSync)
{
  KConfig* config = kapp->getConfig();
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
