#include "kpgp.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#include <qfile.h>
#include <qregexp.h>
#include <qcursor.h>
#include <qstrlist.h>
#include <qlabel.h>
#include <qlined.h>

#include <kapp.h>
#include <kapp.h>
#include <kiconloader.h>


static void
pgpSigHandler(int)
{
  debug("alarm");
}


Kpgp *Kpgp::kpgpObject = 0L;

Kpgp::Kpgp()
  : input(0), signature(0), signatureID(0), keyNeeded(0), persons(),
    publicKeys()
{
  initMetaObject();
  flagNoPGP = TRUE;

  kpgpObject=this;
  havePassPhrase = FALSE;
  storePass = FALSE;
  passPhrase = 0;
  pgpRunning = FALSE;

  // shouldn't we use our own config file for kpgp???
  if(!checkForPGP()) return;
  readConfig();

  // get public keys
  runPGP(PUBKEYS);
}

Kpgp::~Kpgp()
{
  clear(TRUE);
}

// ----------------- public methods -------------------------

void 
Kpgp::readConfig()
{
  KConfig *config = kapp->getConfig();
  config->setGroup("Kpgp");
  storePass = config->readNumEntry("storePass");
  flagEncryptToSelf = config->readNumEntry("encryptToSelf");
  pgpUser = config->readEntry("user");
}

void
Kpgp::writeConfig(bool sync)
{
  KConfig *config = kapp->getConfig();
  config->setGroup("Kpgp");
  config->writeEntry("storePass",storePass);
  config->writeEntry("encryptToSelf",flagEncryptToSelf);
  config->writeEntry("user",pgpUser);

  if(sync)
    config->sync();
}

void
Kpgp::setUser(const QString aUser)
{
  pgpUser = aUser.copy();
}

bool 
Kpgp::setMessage(const QString mess)
{
  int index;

  clear();
  input = mess;

  flagEncrypted = FALSE;
  flagSigned = FALSE;
  flagSigIsGood = FALSE;

  if((index = input.find("-----BEGIN PGP")) != -1)
  {
    if(flagNoPGP)
    {
      errMsg = i18n("Couldn't find PGP executable.\n"
			 "Please check your PATH is set correctly.");
      return FALSE;
    }
    // front and backmatter...
    front = input.left(index);
    index = input.find("-----END PGP",index);
    index = input.find("\n",index+1);
    back  = input.right(input.size() - index - 1);

    prepare(TRUE);
    runPGP(DECRYPT);
    return TRUE;
  }
  //  debug("Kpgp: message does not contain PGP parts");
  return FALSE;
}

const QString 
Kpgp::frontmatter(void) const
{
  return front;
}

const QString
Kpgp::backmatter(void) const
{
  return back;
}

const QString
Kpgp::message(void) const
{
  // do we have a deciphered text?
  if(!output.isEmpty()) return output;

  // no, then return the original one
  debug("Kpgp: No output!");
  return input;
}

bool 
Kpgp::prepare(bool needPassPhrase)
{
  if(flagNoPGP)
  {
    errMsg = i18n("Could not find PGP executable.\n"
		       "Please check your PATH is set correctly.");
    return FALSE;
  }
  if(needPassPhrase)
  {
    if(!havePassPhrase)
      setPassPhrase(askForPass());
    if(!havePassPhrase)
    {
      errMsg = i18n("The pass phrase is missing.");
      return FALSE;
    }
  }
  return TRUE;
}

void
Kpgp::cleanupPass(void)
{
  if(!storePassPhrase())
  {
    passPhrase.replace(QRegExp(".")," ");
    passPhrase = 0;
  }
}

bool 
Kpgp::decrypt(void)
{
  bool retval;

  // do we need to do anything?
  if(!flagEncrypted) return TRUE;
  // everything prepared?
  if(!prepare(TRUE)) return FALSE;
  // ok now try to decrypt the message.
  retval = runPGP(DECRYPT);
  // erase the passphrase if we do not want to keep it
  cleanupPass();

  return retval;
}

bool 
Kpgp::encryptFor(const QStrList& aPers, bool sign)
{
  int action = ENCRYPT;
  QString persStr;
  QStrList* pl;
  char* pers;

  if(!prepare(TRUE)) return FALSE;

  persons.clear();
  pl = (QStrList*)&aPers;
  for(pers=pl->first(); pers; pers=pl->next())
  {
    persStr += "\"";
    persStr += pers;
    persStr += "\" ";
    persons.append(pers);
  }

  if(sign) action += SIGN;
  return runPGP(action, persStr);
}

const QStrList*
Kpgp::keys(void)
{
  if (!prepare()) return NULL;

  runPGP(PUBKEYS);
  return &publicKeys;
}

bool 
Kpgp::havePublicKey(QString _person)
{
  QString str;

  return TRUE; // DEBUG ONLY !!

  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
    if(str.contains(_person)) return TRUE;

  return FALSE;
}

bool 
Kpgp::sign(void)
{
  if (!prepare(TRUE)) return FALSE;
  return runPGP(SIGN);
}

bool 
Kpgp::signKey(QString _key)
{
  if (!prepare(TRUE)) return FALSE;
  return runPGP(SIGNKEY, _key);
}


bool 
Kpgp::isEncrypted(void) const
{
  return flagEncrypted;
}

const QStrList*
Kpgp::receivers(void) const
{
  if (persons.count()<=0) return NULL;
  return &persons;
}

const QString 
Kpgp::KeyToDecrypt(void) const
{
  return keyNeeded;
}

bool
Kpgp::isSigned(void) const
{
  return flagSigned;
}

QString 
Kpgp::signedBy(void) const
{
  return signature;
}

QString 
Kpgp::signedByKey(void) const
{
  return signatureID;
}

bool 
Kpgp::goodSignature(void) const
{
  return flagSigIsGood;
}

void 
Kpgp::setEncryptToSelf(bool flag)
{
  flagEncryptToSelf = flag;
}

bool 
Kpgp::encryptToSelf(void) const
{
  return flagEncryptToSelf;
}

bool 
Kpgp::storePassPhrase(void) const
{
  return storePass;
}

void 
Kpgp::setStorePassPhrase(bool setTo)
{
  storePass = setTo;
}

void 
Kpgp::setPassPhrase(const QString aPass)
{
  if (!aPass.isEmpty())
  {
    passPhrase = aPass;
    havePassPhrase = TRUE;
  }
  else
  {
    if (!passPhrase.isEmpty())
      passPhrase.replace(QRegExp(".")," ");
    passPhrase = 0;
    havePassPhrase = FALSE;
  }
}

bool 
Kpgp::changePassPhrase(const QString /*oldPass*/, 
		       const QString /*newPass*/)
{
  //FIXME...
  warning(i18n("Sorry, but this feature\nis still missing"));
  return FALSE;
}

void 
Kpgp::clear(bool erasePassPhrase)
{
  if(erasePassPhrase && havePassPhrase && !passPhrase.isEmpty())
  {
    passPhrase.replace(QRegExp(".")," ");
    passPhrase = 0;
  }
  // erase strings from memory
#ifdef BROKEN
  input.replace(QRegExp(".")," ");
  signature.replace(QRegExp(".")," ");
  signatureID.replace(QRegExp(".")," ");
  keyNeeded.replace(QRegExp(".")," ");
#endif
  input = 0;
  signature = 0;
  signatureID = 0;
  keyNeeded = 0;
  output = 0;
  info = 0;
  persons.clear();
  
  flagEncrypted = FALSE;
  flagSigned = FALSE;
  flagSigIsGood = FALSE;
}

const QString 
Kpgp::lastErrorMsg(void) const
{
  return errMsg;
}

bool 
Kpgp::havePGP(void) const
{
  return !flagNoPGP;
}



// ------------------ static methods ----------------------------
Kpgp *
Kpgp::getKpgp()
{
  if (!kpgpObject)
  {
    kpgpObject = new Kpgp;
    kpgpObject->readConfig();
  }
  return kpgpObject;
}

const QString
Kpgp::askForPass(QWidget *parent)
{
  return KpgpPass::getPassphrase(parent);
}

const QString 
Kpgp::decode(const QString text, bool returnHTML)
{
  QString deciphered;

  if(text.find("-----BEGIN PGP") == -1) return text;
  Kpgp* pgp=Kpgp::getKpgp();
  pgp->setMessage(text);
  // first check if text is encrypted
  if(!pgp->isEncrypted()) return text;

  //need to decrypt it...
  if(!pgp->havePassPhrase) pgp->askForPass();
  pgp->decrypt();

  // o.k. put it together...
  deciphered = pgp->frontmatter();

  if(!pgp->isSigned()) {
    deciphered += pgp->message();
  } else {
    if(returnHTML) deciphered += "<B>";
    else deciphered += "----- ";
    if(pgp->goodSignature()){
      deciphered += i18n("Message signed by");
    } else {
      deciphered += i18n("Warning"); 
      deciphered += ":";
      deciphered += i18n("Message has a bad signature from");
    }
    deciphered += " " + pgp->signedBy();
    if(returnHTML) deciphered += "</B><BR>";
    else deciphered += " -----\n\n";
    deciphered += pgp->message();
    if(returnHTML) deciphered += "<B><BR>";
    else deciphered += "\n\n----- ";
    deciphered += i18n("End PGP signed message"); 
    if(returnHTML) deciphered += "</B><BR>";
    else deciphered += " -----\n";
  }

  //add backmatter
  deciphered += pgp->backmatter();

  // convert to HTML
  if(returnHTML)
  {
    deciphered.replace(QRegExp("\n"),"<BR>");
    deciphered.replace(QRegExp("\\x20",FALSE,FALSE),"&nbsp"); // SP
  }
  //debug("Kpgp: message is: %s",(const char *)deciphered);

  return deciphered;
}



// --------------------- private functions -------------------

// check if pgp installed
// currently only supports 2.6.x
bool 
Kpgp::checkForPGP(void)
{
  // get path
  QString path;
  QStrList pSearchPaths;
  int index = 0;
  int lastindex = 0;

  flagNoPGP=TRUE;

  path = getenv("PATH");
  while((index = path.find(":",lastindex+1)) != -1)
  {
    pSearchPaths.append(path.mid(lastindex+1,index-lastindex-1));
    lastindex = index;
  }
  if(lastindex != (int)path.size() - 2)
    pSearchPaths.append( path.mid(lastindex+1,path.size()-lastindex-1) );

  QStrListIterator it(pSearchPaths);

  while ( it.current() )
  {
    path = it.current();
    path += "/pgp";
    if ( !access( path, X_OK ) )
    {
      flagNoPGP=FALSE;
      //debug("Kpgp: found pgp");
      return TRUE;
    }
    ++it;
  }
  debug("Kpgp: didn't find pgp");
  return FALSE;
}

bool
Kpgp::runPGP(int action, const char* args)
{
  int len, rc;
  char str[1024];
  bool addUserId = FALSE;
  QString cmd, tmpName(256), inName, outName, errName;
  int infd, outfd, errfd;
  void (*oldsig)(int);

  cmd = "pgp +batchmode";

  switch (action)
  {
  case ENCRYPT:
    cmd += " -ea";
    break;
  case SIGN:
    cmd += " -sat";
    addUserId=TRUE;
    break;
  case ENCSIGN:
    cmd += " -seat";
    addUserId=TRUE;
    break;
  case SIGNKEY:
    cmd += " -ks";
    addUserId=TRUE;
    break;
  case DECRYPT:
  case TEST:
    break;
  case PUBKEYS:
    cmd += " -kv";
    break;
  default:
    warning("kpgp: wrong action given to runPGP()");
    return false;
  }

  if(addUserId && !pgpUser.isEmpty())
  {
    cmd += " -u \"";
    cmd += pgpUser;
    cmd += '"';
  }

  // add additional arguments
  if (args)
  {
    cmd += ' ';
    cmd += args;
  }

  // add passphrase
  if(havePassPhrase)
  {
    sprintf(str," \"-z%s\"",(const char *)passPhrase);
    cmd += str;
  }
  cmd += " -f";

  tmpName.sprintf("/tmp/.kmail-");
  inName  = tmpName + "in";
  outName = tmpName + "out";
  errName = tmpName + "err";

  cmd = cmd + " <"+inName+" >"+outName+" 2>"+errName;

  infd = open(inName.data(), O_RDWR|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE);
  if (!input.isEmpty()) write(infd, input.data(), input.length());
  close(infd);

  oldsig = signal(SIGALRM,pgpSigHandler);
  alarm(5);
  rc = system(cmd.data());
  alarm(0);
  signal(SIGALRM,oldsig);
  debug("pgp: system() rc=%d. Reading results", rc);

  output = 0;
  outfd = open(outName.data(), O_RDONLY);  
  if (outfd >= 0) 
  {
    while ((len=read(outfd,str,1023))>0)
    {
      str[len] ='\0';
      output += str;
    }
    close(outfd);
  }

  info = 0;
  errfd = open(errName.data(), O_RDONLY);  
  if (errfd >= 0) 
  {
    while ((len=read(errfd,str,1023))>0)
    {
      str[len] ='\0';
      info += str;
    }
    close(errfd);
  }

  unlink(inName.data());
  unlink(outName.data());
  unlink(errName.data());

  return parseInfo(action);
}

bool Kpgp::parseInfo(int action)
{
  bool returnFlag = TRUE;
  int index, index2;
  switch (action)
  {
  case DECRYPT:
  case TEST:
    if( info.find("File contains key") != -1)
    {
      output = "";
      // FIXME: should do something with it...
    }

    if( info.find("Bad pass phrase") != -1)
    {
      //	       debug("Kpgp: isEncrypted");
      if(action == DECRYPT) 
      {
	errMsg = i18n("Bad pass Phrase; couldn't decrypt");
	havePassPhrase = FALSE;
	returnFlag = FALSE;
      }
      flagEncrypted = TRUE;
      // check for persons
      index = info.find("can only be read by:");
      if(index != -1) 
      {
	index = info.find("\n",index);
	int end = info.find("\n\n",index);

	persons.clear();
	while( (index2 = info.find("\n",index+1)) <= end )
	{
	  QString item = info.mid(index+1,index2-index-1);
	  item.stripWhiteSpace();
	  persons.append(item);
	  index = index2;
	}
      }
    }
    if((index = info.find("File has signature")) != -1)
    {
      flagSigned = TRUE;
      flagSigIsGood = FALSE;
      if( info.find("Key matching expected") != -1)
      {
	index = info.find("Key ID ",index);
	signatureID = info.mid(index+7,8);
	signature = "unknown key ID " + signatureID + " ";
	// FIXME: not a very good solution...
	flagSigIsGood = TRUE;
      }
      else
      {
	if( info.find("Good signature") != -1 )
	  flagSigIsGood = TRUE;
		    
	// get signer
	index = info.find("\"",index);
	index2 = info.find("\"", index+1);
	signature = info.mid(index+1, index2-index-1);
		    
	// get key ID of signer
	index = info.find("key ID ",index2);
	signatureID = info.mid(index+7,8);
      }
    }
    if(info.find("Pass phrase is good") != -1)
      flagEncrypted = FALSE;
    break;

  case ENCRYPT:
  case ENCSIGN:
  {
    index = 0;
    bool bad = FALSE;
    QString badkeys = "";
    while((index = info.find("Cannot find the public key",index)) 
	  != -1)
    {
      bad = TRUE;
      index = info.find("'",index);
      index2 = info.find("'",index+1);
      badkeys += info.mid(index, index2-index+1) + ' ';
    }
    if(bad)
    {
      badkeys.stripWhiteSpace();
      badkeys.replace(QRegExp(" "),", ");
      errMsg.sprintf("Could not find public keys matching the\n" 
		     "userid(s) %s. These persons won't be able\n"
		     "to read the message.",
		     (const char *)badkeys);
      returnFlag = FALSE;
    }
    break;
  }
  case SIGN:
    if(info.find("Pass phrase is good") != -1)
    {
      flagEncrypted = TRUE;
    }
    if( info.find("Bad pass phrase") != -1)
    {
      errMsg = i18n("Bad pass Phrase; couldn't sign");
      returnFlag = FALSE;
      havePassPhrase = FALSE;
    }
    break;
  case PUBKEYS:
    publicKeys.clear();
    index = output.find("\n",1)+1; // skip first to "\n"
    while( (index = output.find("\n",index)) != -1)
    {
      //parse line
      QString line;
      if( (index2 = output.find("\n",index+1)) != -1)
	// skip last line
      {
	int index3 = output.find("pub ",index);
		    
	if( (index3 >index2) || (index3 == -1) )
	{
	  // second adress for the same key
	  line = output.mid(index+1,index2-index-1);
	  line = line.stripWhiteSpace();	       
	} else {
	  // line with new key
	  int index3 = output.find(
	    QRegExp("/[0-9][0-9]/[0-9][0-9] "),
	    index);
	  line = output.mid(index3+7,index2-index3-7);
	}
	//debug("kpgp: found key for %s",(const char *)line);
	publicKeys.append(line);
      }
      index = index2;
    }
    break;
  default:
    warning("Kpgp: bad action %d in parseInfo()", action);
    returnFlag = FALSE;
  }

  return returnFlag;
}

// -----------------------------------------------------------------------

KpgpPass::KpgpPass(QWidget *parent, const char *name)
  : QDialog(parent, 0, TRUE)
{
  KIconLoader* loader = kapp->getIconLoader();
  QPixmap pixm;

  setCaption(name);
  setFixedSize(264,80);
  cursor = kapp->overrideCursor();
  if(cursor != 0)
    kapp->setOverrideCursor(QCursor(ibeamCursor));
  this->setCursor(QCursor(ibeamCursor));
  QLabel *text = new QLabel(i18n("Please enter your\nPGP passphrase"),this);
  text->move(56,4);
  text->setAutoResize(TRUE);
  QLabel *icon = new QLabel(this);
  pixm = loader->loadIcon("pgp-keys.xpm");
  icon->setPixmap(pixm);
  icon->move(4,8);
  icon->resize(48,48);

  lineedit = new QLineEdit(this);
  lineedit->setEchoMode(QLineEdit::Password);
  lineedit->move(56, 8+text->size().height());
  lineedit->resize(200, lineedit->size().height());
  lineedit->setFocus();

  connect(lineedit,SIGNAL(returnPressed()),this,SLOT(accept()) );  
}

KpgpPass::~KpgpPass()
{
  if(cursor != 0)
    kapp->restoreOverrideCursor();
}

QString 
KpgpPass::getPassphrase(QWidget *parent)
{
  KpgpPass kpgppass(parent, i18n("PGP Security Check"));
  kpgppass.exec();
  return kpgppass.getPhrase().copy();
}

QString
KpgpPass::getPhrase()
{
  return lineedit->text();
}


// ------------------------------------------------------------------------
#include "kpgp.moc"

