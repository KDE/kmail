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
#include <kiconloader.h>


/* TODO :
  --  When there is no output from PGP, a warning dialog should
      appear (when there's a problem now, the MESSAGE IS SENT
      UNENCRYPTED AND/OR UNSIGNED !)
  --  Use sending of passphrase via pipe, rather than via -z,
      because it can be listed with 'ps' then. -- not very safe
*/ 

/* CHANGES by Juraj Bednar <bednar@isternet.sk>

  -- Support for PGP 5.0 added. PGP 2.6.3 should work, but I have
     no way to test it out
  -- /tmp/.kmail-* files are no longer used. Instead we use pipes
     (they are much safer and there will be no conflict with other
     kmails running)

*/

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
  bool isSigned;

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

    // test if the message is encrypted or signed
    isSigned = (strcmp(input.data() + index + 15, "SIGNED ")==0);

    // front and backmatter...
    front = input.left(index);
    index = input.find("-----END PGP",index);
    index = input.find("\n",index+1);
    back  = input.right(input.size() - index - 1);

    if (!flagPgp50)
    prepare(isSigned); else prepare(1);
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
Kpgp::testSign(void)
{
  // everything prepared?
  if(!prepare(FALSE)) return FALSE;
  // ok now try to decrypt the message.
  return runPGP(ENCSIGN);
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
      if (flagPgp50) 
       persStr += "-r \""; else
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
  bool isSigned, decryptOk;
  int i = text.find("-----BEGIN PGP ");
  if (i == -1) return text;

  // test if the message is encrypted or signed
  isSigned = (strcmp(((const char*)text)+i+15, "SIGNED ")==0);

  Kpgp* pgp=Kpgp::getKpgp();
  pgp->setMessage(text);

  // first check if text is encrypted
  if(!pgp->isEncrypted()) return text;

  //need to decrypt it...
  if(!isSigned && !pgp->havePassPhrase) pgp->askForPass();
  decryptOk = pgp->decrypt();

  // o.k. put it together...
  deciphered = pgp->frontmatter();

  if(returnHTML) deciphered += "<B>";
  else deciphered += "----- ";

  if(!pgp->isSigned()) {
    if (decryptOk) deciphered += i18n("Encrypted message");
    else {
      deciphered +=  i18n("Cannot decrypt message:");
      deciphered += " ";
      deciphered += pgp->lastErrorMsg();
    }
  } else {
    if(pgp->goodSignature()){
      deciphered += i18n("Message signed by");
    } else {
      deciphered += i18n("Warning"); 
      deciphered += ":";
      deciphered += i18n("Message has a bad signature from");
    }
    deciphered += " " + pgp->signedBy();
  }

  if(returnHTML) deciphered += "</B><BR>";
  else deciphered += " -----\n\n";
  
  deciphered += pgp->message();

  if(returnHTML) deciphered += "<B><BR>";
  else deciphered += "\n\n----- ";

  if (pgp->isSigned()) deciphered += i18n("End PGP signed message");
  else deciphered += i18n("Encrypted message");

  if(returnHTML) deciphered += "</B><BR>";
  else deciphered += " -----\n";

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

bool
Kpgp::executePGP(QString cmd, int *in, int *out, int *err)
{
           int pin[2], pout[2], perr[2], child_pid;

         pipe(pin);
         pipe(pout);
         pipe(perr);

         if(!(child_pid = fork()))
         {
           /*We're the child.*/
           close(pin[1]);    
           dup2(pin[0], 0); /* pipe to stdin (0) */
           close(pin[0]);

           close(pout[0]);
           dup2(pout[1], 1); /* pipe to stdout (1) */
           close(pout[1]);

           close(perr[0]);
           dup2(perr[1], 2);  /* pipe to stderr (2) */
           close(perr[1]);

           execl("/bin/sh", "sh", "-c", cmd.data(), NULL);
           _exit(127);
         }

         /*Only get here if we're the parent.*/

         close(perr[1]);
         *err = perr[0];
         close(pout[1]);
         *out = pout[0];
         close(pin[0]);
         *in = pin[1];

         return TRUE;
}

// check if pgp installed
// currently only supports 2.6.x
// And now 5.x at alpha stage ;-). Please test ! I'll do too. - Juraj
bool 
Kpgp::checkForPGP(void)
{
  /* Check for pgp 5.0i */
  int rc = system("pgpe -h 2>/dev/null >/dev/null");

  flagNoPGP = FALSE;

  if (!((rc != -1) && (rc != 127))) 
  {
    /* Oh no ! PGP 5.x not found. Check for 2.6.x instead */
    flagPgp50 = FALSE;
    rc = system("pgp -h 2>/dev/null >/dev/null");
    if ((rc != -1) && (rc != 127)) 
       flagNoPGP = TRUE;
  } else flagPgp50 = TRUE;

  return flagNoPGP;
}

bool
Kpgp::runPGP(int action, const char* args)
{
  int len;
  char str[1024];
  bool addUserId = FALSE;
  QString cmd;
  int infd, outfd, errfd;
  void (*oldsig)(int);

  if (!flagPgp50) {
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
 } else {
  switch (action)
  {
  case ENCRYPT:
    cmd = "pgpe -a";
    break;
  case SIGN:
    cmd = "pgps -at";
    addUserId=TRUE;
    break;
  case ENCSIGN:
    cmd = "pgpe -sat";
    addUserId=TRUE;
    break;
  case SIGNKEY:
    cmd = "pgpk -s";
    addUserId=TRUE;
    break;
  case DECRYPT:
  case TEST:
    cmd = "pgpv";
    break;
  case PUBKEYS:
    cmd = "pgpk -l";
    break;
  default:
    warning("kpgp: wrong action given to runPGP()");
    return false;
  }

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
    sprintf(str," '-z%s'",(const char *)passPhrase);
    cmd += str;
  }
  cmd += " -f";
  if (flagPgp50) 
    cmd += " +batchmode=1 +OutputInformationFD=2";

  executePGP(cmd,&infd,&outfd,&errfd);

  if (!input.isEmpty()) {
    write(infd, input.data(), input.length());
    }
  close(infd);

  output = 0;

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

  if (errfd >= 0) 
  {
    while ((len=read(errfd,str,1023))>0)
    {
      str[len] ='\0';
      info += str;
    }
    close(errfd);
  }


  /* Clean up */
  oldsig = signal(SIGALRM,pgpSigHandler);
  alarm(5);
  wait(NULL);
  alarm(0);
  signal(SIGALRM,oldsig);
  unsetenv("PGPPASSFD");
  debug("Okay !");

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

    if( (info.find("Bad pass phrase") != -1) || (info.find("Need a pass phrase") != -1))
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
      if (flagPgp50)
        index = info.find("can only be decrypted by:"); else
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
    if(((index = info.find("File has signature")) != -1) || flagPgp50)
    {
      if (!flagPgp50) flagSigned = TRUE;
      flagSigIsGood = FALSE;
      if( info.find("Key matching expected") != -1)
      {
	index = info.find("Key ID ",index);
	signatureID = info.mid(index+7,8);
	signature = "unknown key ID " + signatureID + " ";
	// FIXME: not a very good solution...
	flagSigIsGood = TRUE;
	flagSigned = TRUE;
      }
      else
      {
	if( info.find("Good signature") != -1 ) {
	  flagSigned = TRUE;
	  flagSigIsGood = TRUE;
        }
		    
	if (flagPgp50) index= info.find("Good signature");
	// get signer
	index = info.find("\"",index);
	index2 = info.find("\"", index+1);

	if (index>=0 && index2>=0)
	  signature = info.mid(index+1, index2-index-1);
	else signature = i18n("Unknown");
		    
	// get key ID of signer
        if (flagPgp50)
	  index = info.find("Key ID ",info.find("Good signature")); else
	  index = info.find("key ID ",index2);
	
	signatureID = info.mid(index+7,8);
      }
    }
    if(info.find("Pass phrase is good") != -1)
      flagEncrypted = FALSE;
    if(info.find("Message is encrypted") != -1)
      flagEncrypted = TRUE;
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
    if((info.find("Pass phrase is good") != -1) || flagPgp50)
    {
      flagEncrypted = TRUE;
    }
    if( (info.find("Bad pass phrase") != -1) || (info.find("Need a pass phrase") != -1))
    {
      errMsg = i18n("Bad pass Phrase; couldn't sign");
      returnFlag = FALSE;
      havePassPhrase = FALSE;
      flagEncrypted=FALSE;
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

