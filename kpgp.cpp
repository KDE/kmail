// kpgp.cpp

#include "kpgp.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#include <qfile.h>
#include <qregexp.h>
#include <qcursor.h>
#include <qstrlist.h>

#include <kprocess.h>
#include <kapp.h>
#include <klocale.h>


Kpgp *Kpgp::kpgpObject = 0L;

#define translate(X) klocale->translate(X) 

Kpgp::Kpgp()
  : QObject(), message(0), signature(0), signatureID(0), keyNeeded(0)
{
  // shouldn't we use our own config file for kpgp???
  config = kapp->getConfig();
     
  checkForPGP();
  readConfig();
  kpgpObject=this;
  havePassPhrase = FALSE;
  passPhrase = 0;
  persons = 0;
}

Kpgp::~Kpgp()
{
  clear(TRUE);
}

// ----------------- public methods -------------------------

void 
Kpgp::readConfig()
{
  config->setGroup("Kpgp");
  storePass = config->readNumEntry("storePass");
  flagEncryptToSelf = config->readNumEntry("encryptToSelf");
  secKey = config->readEntry("secretKey");
}

void
Kpgp::writeConfig(bool sync)
{
  config->setGroup("Kpgp");
  config->writeEntry("storePass",storePass);
  config->writeEntry("encryptToSelf",flagEncryptToSelf);
  config->writeEntry("secretKey",secKey);

  if(sync)
    config->sync();
}
 
bool 
Kpgp::setMessage(const QString mess)
{
  clear();
  message = mess;
  debug("Kpgp: set message to: %s",(const char *)mess);

  flagEncrypted = FALSE;
  flagSigned = FALSE;
  flagSigIsGood = FALSE;

  if(message.contains("-----BEGIN PGP"))
  {
    runPGP();
    return TRUE;
  }
  return FALSE;
}

QString 
Kpgp::getMessage()
{
  // do we have a deciphered text?
  if(!output.isEmpty())
    return output;

  // no, then return the original one
  return message;
}

bool 
Kpgp::decrypt(QString *pass = 0)
{
  bool retval;

  setPassPhrase(pass);
  // do we need to do anything?
  if(!flagEncrypted) return TRUE;

  if(!havePassPhrase)
  {
    errMsg = translate("No Pass Phrase. Couldn't decrypt.");
    return FALSE;
  }

  if(flagNoPGP)
  {
    errMsg = translate("Couldn't find PGP executable.\n"
		       "Please check your PATH is set correctly.");
    return FALSE;
  }

  // ok now try to decrypt the message.
  retval = runPGP(passPhrase, DECRYPT);
     
  // delete passphrase if it shouldn't be stored
  if (!storePassPhrase && passPhrase != 0) {
    passPhrase->replace(QRegExp(".")," ");
  }

  return retval;
}

bool 
Kpgp::encryptTo(QStrList *pers, bool sign = TRUE, QString *pass = 0)
{
  int action = ENCRYPT;

  assert(pers != 0);
  persons = pers;

  setPassPhrase(pass);

  if((!havePassPhrase) && sign)
  {
    errMsg = translate("No Passphrase. Can't sign message");
    return FALSE;
  }

  if(sign)
  {
    action += SIGN;
    return runPGP(passPhrase, action, pers);
  }
  else
  {
    return runPGP(0, action, pers);
  }
}

bool 
Kpgp::sign(QString *pass = 0)
{
  setPassPhrase(pass);
  return runPGP(passPhrase, SIGN);
}

bool 
Kpgp::isEncrypted()
{
  return flagEncrypted;
}

QStrList 
Kpgp::isEncryptedFor()
{
  return persons;
}

QString 
Kpgp::KeyToDecrypt()
{
  return keyNeeded;
}

bool 
Kpgp::isSigned()
{
  return flagSigned;
}

QString 
Kpgp::signedBy()
{
  return signature;
}

QString 
Kpgp::signedByKey()
{
  return signatureID;
}

bool 
Kpgp::goodSignature()
{
  return flagSigIsGood;
}

void 
Kpgp::setEncryptToSelf(bool flag)
{
  flagEncryptToSelf=flag;
}

bool 
Kpgp::encryptToSelf()
{
  return flagEncryptToSelf;
}

bool 
Kpgp::storePassPhrase(void)
{
  return storePass;
}

void 
Kpgp::setStorePassPhrase(bool setTo)
{
  storePass = setTo;
}

void 
Kpgp::setPassPhrase(QString *pass)
{
  if(pass != 0)
  {
    if(passPhrase != 0)
    {
      passPhrase->replace(QRegExp(".")," ");
      delete passPhrase;
    }
    havePassPhrase = TRUE;
    passPhrase = pass;
    pass = 0;
  }
}

bool 
Kpgp::changePassPhrase(QString *oldPassPhrase, QString
		       *newPassPhrase)
{
  //FIXME...
  warning(translate("Sorry, but this feature\nis still missing"));
  return FALSE;
}

void 
Kpgp::setSecretKey(const QString Key)
{
  secKey = Key;
}

const QString 
Kpgp::secretKey(void)
{
  return secKey;
}

void 
Kpgp::clear(bool erasePassPhrase = FALSE)
{
  if(erasePassPhrase && havePassPhrase && passPhrase != 0) {
    CHECK_PTR(passPhrase);
    passPhrase->replace(QRegExp(".")," ");
    passPhrase = 0;
  }
#ifdef BROKEN
  // erase strings from memory
  message.replace(QRegExp(".")," ");
  signature.replace(QRegExp(".")," ");
  signatureID.replace(QRegExp(".")," ");
  keyNeeded.replace(QRegExp(".")," ");
#endif
  message = "";
  signature = "";
  signatureID = "";
  keyNeeded = "";
  output = "";
     
  if(persons != 0)
    delete persons;
  persons = 0;
  
  flagEncrypted = FALSE;
  flagSigned = FALSE;
  flagSigIsGood = FALSE;
}

const QString 
Kpgp::lastErrorMsg()
{
  return errMsg;
}

bool 
Kpgp::havePGP()
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

QString *
Kpgp::askForPass()
{
  return KpgpPass::getPassphrase();
}

QString 
Kpgp::decode(const QString text, bool returnHTML=FALSE)
{
  QString deciphered;
  int pos;

  if((pos = text.find("-----BEGIN PGP")) == -1) return text;
  Kpgp pgp;
  pgp.setMessage(text);
  // first check if text is encrypted
  if(pgp.isEncrypted())
  {
    //need to decrypt it...
    if(!pgp.havePassPhrase){
      pgp.decrypt(pgp.askForPass());
    } else {
      pgp.decrypt();
    }
  }

  if(pgp.isEncrypted()) return text;

  // o.k. put it together...
  deciphered = "\n";
  //    deciphered = text;
  //    deciphered.truncate(pos);
  if(returnHTML)
  {
    if(pgp.isSigned())
    {
      if(pgp.flagSigIsGood){
	deciphered += "<B>";
	deciphered += translate("Message signed by");
      } else {
	deciphered += "<B>";
	deciphered += translate("Warning"); 
	deciphered += ":";
	deciphered += translate("Message has a bad signature from");
      }
      deciphered += " " + pgp.signedBy() + "</B><BR>";
      deciphered += pgp.getMessage();
      deciphered += "<BR><B>"; 
      deciphered += translate("End PGP signed message"); 
      deciphered += "</B><BR>";
    }
    else {
      deciphered += pgp.getMessage();
    }
  }
  else 
  {
    if(pgp.isSigned())
    {
      if(pgp.flagSigIsGood){
	deciphered += "----- ";
	deciphered += translate("Message signed by");
      } else {
	deciphered += "----- ";
	deciphered += translate("Warning"); 
	deciphered += ":";
	deciphered += translate("Message has a bad signature from");
      }
      deciphered += " " + pgp.signedBy() + " -----\n\n";
      deciphered += pgp.getMessage();
      deciphered += "\n\n----- ";
      deciphered += translate("End PGP signed message");
      deciphered += " -----\n";
    }
    else {
      deciphered += pgp.getMessage();
    }
  }
  //add backmatter
  pos = text.find("-----END");
  pos = text.find("\n",pos);
  deciphered += text.right(text.size()-pos-1);

  // convert to HTML
  if(returnHTML == TRUE)
  {
    deciphered.replace(QRegExp("\n"),"<BR>");
    deciphered.replace(QRegExp("\\x20",FALSE,FALSE),"&nbsp"); // SP
  }
  debug("Kpgp: message is: %s",(const char *)deciphered);

  return deciphered;
}



// --------------------- private functions -------------------

// check if pgp installed
// currently only supports 2.6.x
bool 
Kpgp::checkForPGP()
{
  // get path
  QString path;
  QStrList *pSearchPaths = new QStrList();;
  int index = 0;
  int lastindex = 0;

  flagNoPGP=TRUE;

  path = getenv("PATH");
  while((index = path.find(":",lastindex+1)) != -1)
  {
    pSearchPaths->append(path.mid(lastindex+1,index-lastindex-1));
    lastindex = index;
  }
  if(lastindex != (int)path.size() - 2)
    pSearchPaths->append( path.mid(lastindex+1,path.size()-lastindex-1) );

  QStrListIterator it( *pSearchPaths );

  while ( it.current() )
  {
    path = it.current();
    path += "/pgp";
    if ( !access( path, X_OK ) )
    {
      flagNoPGP=FALSE;
      debug("Kpgp: found pgp");
      return TRUE;
    }
    ++it;
  }
  debug("Kpgp: didn't find pgp");
  return FALSE;
}


bool 
Kpgp::runPGP(const QString *pass = 0, 
	     int action = TEST, QStrList *args = 0)
{
  KProcess proc;
  proc.setExecutable("pgp");
  proc << "+batchmode";

  switch (action)
  {
  case ENCRYPT:
    proc << "-ea";
    break;
  case SIGN:
    proc << "-sat";
    if(secKey != "" && secKey != 0) proc << "-u" << secKey;
    break;
  case (ENCSIGN):
    proc << "-seat";
    if(secKey != "" && secKey != 0) proc << "-u" << secKey;
    break;
  case DECRYPT:
  case TEST:
    break;
  default:
    warning("kpgp: wrong action given to runPGP()");
  }

  // add additional arguments
  if(args != 0)
  {
    char *item;
    if( (item = args->first()) != 0)
      proc << item;
    while( (item = args->next()) != 0 )
      proc << item;
  }

  if( pass != 0 )
  {
    // debug("Kpgp: runPGP: passphrase is: %s",(const char *)*pass);
    QString phrase;
    phrase.sprintf("-z%s",(const char *)*pass);
    //debug("Kpgp: phrase is: %s",(const char *)phrase);
    proc << phrase;
  }
  proc << "-f";

  // prepare stdin, stdout, stderr
  info = "";
  output = "";

  connect(&proc,SIGNAL(receivedStdout(KProcess *, char *, int)), 
	  this,SLOT(runPGPOut(KProcess *, char *, int)) );
  connect(&proc,SIGNAL(receivedStderr(KProcess *, char *, int)), 
	  this,SLOT(runPGPInfo(KProcess *, char *, int)) );

  connect(&proc,SIGNAL(processExited(KProcess *)), 
	  this,SLOT(runPGPfinished(KProcess *)) );
  connect(&proc,SIGNAL(wroteStdin(KProcess *)),
	  this,SLOT(runPGPcloseStdin(KProcess *)) );
  pgpNotFinished = TRUE;

  debug("Kpgp: starting pgp");
  proc.start(KProcess::NotifyOnExit,KProcess::All);
  proc.writeStdin(strdup(message), message.size()-1);

  // we have to block here, because we want everything
  // to be ready,when the function returns.
  // Anyway, running pgp shouldn't take too long...
  // first wait till pgp read its input. Then close stdin 
  // and wait for the output
  while(pgpNotFinished)
    kapp->processEvents();
    
  debug("Kpgp: after blocking loop");
  //     debug("Kpgp: Output is:\n%s\n",(const char *)output);
  //printf("Kpgp: Info is:\n%s\n",(const char *)info);

  // ok. pgp is ready. now parse the returned info.
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
	errMsg = translate("Bad pass Phrase; couldn't decrypt");
	returnFlag = FALSE;
      }
      flagEncrypted = TRUE;
      // check for persons
      index = info.find("can only be read by:");
      if(index != -1) 
      {
	index = info.find("\n",index);
	int end = info.find("\n\n",index);

	if(persons != 0) delete persons;
	persons = new QStrList();
	while( (index2 = info.find("\n",index+1)) <= end )
	{
	  QString item = info.mid(index+1,index2-index-1);
	  item.stripWhiteSpace();
	  persons->append(item);
	  index = index2;
	}
      }
    }
    if((index = info.find("File has signature")) != -1)
    {
      debug("Kpgp: isSigned");
      flagSigned = TRUE;
      flagSigIsGood = FALSE;
      if( info.find("Key matching expected") != -1)
      {
	index = info.find("Key ID ",index);
	signatureID = info.mid(index+7,8);
	signature = "unknown key ID " + signatureID + " ";
	debug("signatureID=%s",(const char *)signatureID);
	// not a very good solution...
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
	debug("signatureID=%s",(const char *)signatureID);
      }
    }
    if(info.find("Pass phrase is good") != -1)
    {
      debug("Kpgp: Good Passphrase!");
      flagEncrypted = FALSE;
    }
    break;

  case ENCRYPT:
  case (ENCSIGN):
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
    }
  case SIGN:
    if(info.find("Pass phrase is good") != -1)
    {
      debug("Kpgp: Good Passphrase!");
      flagEncrypted = TRUE;
    }
    if( info.find("Bad pass phrase") != -1)
    {
      errMsg = translate("Bad pass Phrase; couldn't sign");
      returnFlag = FALSE;
    }
    break;
  default:
    warning("Kpgp: bad action in parseInfo()");
    returnFlag = FALSE;
  }

  return returnFlag;
}


// ------------------------ slots ---------------------------------

void 
Kpgp::runPGPInfo(KProcess *, char *buf, int buflen)
{
  int count = 0;

  while(count < buflen - 1)
  {
    info += buf[count];
    count++;
  }
}

void 
Kpgp::runPGPOut(KProcess *, char *buf, int buflen)
{
  int count = 0;
  debug("Kpgp: in runPGPOut");

  while(count < buflen - 1)
  {
    output += buf[count];
    count++;
  }
}

void 
Kpgp::runPGPfinished(KProcess *)
{
  debug("Kpgp: pgp has finished");
  pgpNotFinished = FALSE;
}

void 
Kpgp::runPGPcloseStdin(KProcess *proc)
{
  debug("Kpgp: wrote stdin"); 
  proc->closeStdin();
}



// -----------------------------------------------------------------------
KpgpPass::KpgpPass()
  : QWidget()
{
  gotPassphrase = FALSE;

  setFixedSize(200,80);
  this->setCursor(QCursor(ibeamCursor));
  text = new QLabel(translate("Please enter passphrase"),this);
  text->move(30,20);
  text->setAutoResize(TRUE);
  text->show();
  lineedit = new QLineEdit(this);
  lineedit->setEchoMode(QLineEdit::Password);
  lineedit->move(30,40);
  lineedit->show();
  connect(lineedit,SIGNAL(returnPressed()),this,SLOT(passphraseEntered()) );  

  show();
  //this->setActiveWindow();
}

KpgpPass::~KpgpPass()
{
  delete text;
  delete lineedit;
}

QString *
KpgpPass::getPassphrase()
{
  KpgpPass kpgppass;
  while(kpgppass.gotPassphrase == FALSE)
    kapp->processEvents();
  QString *passphrase = new QString();
  *passphrase = kpgppass.pass;
  //debug("Kpgp: Passphrase is: %s",(const char *)*passphrase);

  return passphrase;
}

void 
KpgpPass::passphraseEntered()
{
  pass = lineedit->text();
  gotPassphrase = TRUE;
}


#include "kpgp.moc"
