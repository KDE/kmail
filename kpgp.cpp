#include "kpgp.moc"
#include "kpgpbase.h"

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

#include <qregexp.h>
#include <qcursor.h>
#include <qlabel.h>
#include <qlined.h>
#include <qchkbox.h>
#include <qgrpbox.h>
#include <qlayout.h>

#include <kapp.h>
#include <ksimpleconfig.h>
#include <kiconloader.h>
#include <kmsgbox.h>

Kpgp *Kpgp::kpgpObject = 0L;

Kpgp::Kpgp()
  : publicKeys()
{
  kpgpObject=this;

  // open kpgp config file
  QString aStr = kapp->localconfigdir();
  aStr += "/kpgprc";
  config = new KSimpleConfig(aStr);

  init();
}

Kpgp::~Kpgp()
{
  clear(TRUE);
  delete config;
}

// ----------------- public methods -------------------------

void 
Kpgp::init()
{
  havePassPhrase = FALSE;
  passphrase = 0;

  // do we have a pgp executable
  checkForPGP();

  if(havePgp)
  {
    if(havePGP5)
      pgp = new KpgpBase5();
    else
      pgp = new KpgpBase2();
  }
  else
  {
    // dummy handler
    pgp = new KpgpBase();
    return;
  }

  // read kpgp config file entries
  readConfig();

  // get public keys
  publicKeys = pgp->pubKeys();
}

void 
Kpgp::readConfig()
{
  storePass = config->readBoolEntry("storePass");
}

void
Kpgp::writeConfig(bool sync)
{
  config->writeEntry("storePass",storePass);

  pgp->writeConfig();

  if(sync)
    config->sync();
}

void
Kpgp::setUser(const QString aUser)
{
  pgp->setUser(aUser);
}

const QString 
Kpgp::user(void) const 
{ 
  return pgp->user(); 
}

void 
Kpgp::setEncryptToSelf(bool flag)
{
  pgp->setEncryptToSelf(flag);
}

bool 
Kpgp::encryptToSelf(void) const
{
  return pgp->encryptToSelf();
}

bool 
Kpgp::storePassPhrase(void) const
{
  return storePass;
}

void 
Kpgp::setStorePassPhrase(bool flag)
{
  storePass = flag;
}


bool 
Kpgp::setMessage(const QString mess)
{
  int index;
  int retval = 0;

  clear();

  if(havePgp)
    retval = pgp->setMessage(mess);
  if((index = mess.find("-----BEGIN PGP")) != -1)
  {
    if(!havePgp)
    {
      errMsg = i18n("Couldn't find PGP executable.\n"
			 "Please check your PATH is set correctly.");
      return FALSE;
    }
    if(retval != 0)
      errMsg = pgp->lastErrorMessage();

    // front and backmatter...
    front = mess.left(index);
    index = mess.find("-----END PGP",index);
    index = mess.find("\n",index+1);
    back  = mess.right(mess.size() - index - 1);

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
  return pgp->message();
}

bool 
Kpgp::prepare(bool needPassPhrase)
{
  if(!havePgp)
  {
    errMsg = i18n("Could not find PGP executable.\n"
		       "Please check your PATH is set correctly.");
    return FALSE;
  }

  if((pgp->getStatus() & KpgpBase::NO_SEC_KEY))
    return FALSE;

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
  if(!storePass)
  {
    passphrase.replace(QRegExp(".")," ");
    passphrase = 0;
    havePassPhrase = false;
  }
}

bool 
Kpgp::decrypt(void)
{
  int retval;

  // do we need to do anything?
  if(!pgp->isEncrypted()) return TRUE;
  // everything ready
  if(!prepare(TRUE)) return FALSE;
  // ok now try to decrypt the message.
  retval = pgp->decrypt(passphrase);
  // erase the passphrase if we do not want to keep it
  cleanupPass();

  if((retval & KpgpBase::BADPHRASE))
  {
    //debug("Kpgp: bad passphrase");
    havePassPhrase = false;
  }

  if(retval & KpgpBase::ERROR) 
  {
    errMsg = pgp->lastErrorMessage();
    return false;
  }
  return true;
}

bool 
Kpgp::sign(void)
{
  return encryptFor(0, true);
}

bool 
Kpgp::encryptFor(const QStrList& aPers, bool sign)
{
  QStrList persons, noKeyFor;
  char* pers;
  int status = 0;
  errMsg = "";
  int ret;
  QString aStr;

  persons.clear();
  noKeyFor.clear();

  if(!aPers.isEmpty()) 
  {
    QStrListIterator it(aPers);
    while((pers = it.current()))
    {
      QString aStr = getPublicKey(pers);
      if(!aStr.isEmpty()) 
        persons.append(aStr);
      else
	noKeyFor.append(pers);
      ++it;
    }
    if(persons.isEmpty())
    {
      int ret = KMsgBox::yesNo(0,i18n("PGP Warning"),
			       i18n("Could not find the public keys for the\n" 
				    "recipients of this mail.\n"
				    "Message will not be encrypted."),
			       KMsgBox::EXCLAMATION, 
			       i18n("Continue"), i18n("Cancel"));
      if(ret == 2) return false;
    }
    else if(!noKeyFor.isEmpty())
    {
      QString aStr = i18n("Could not find the public keys for\n");
      QStrListIterator it(noKeyFor);
      aStr += it.current();
      ++it;
      while((pers = it.current()))
      {
	aStr += ",\n";
	aStr += pers;
	++it;
      }
      if(it.count() > 1)
	aStr += i18n("These persons will not be able to\n");
      else
	aStr += i18n("This person will not be able to\n");

      aStr += i18n("decrypt the message.");
      int ret = KMsgBox::yesNo(0,i18n("PGP Warning"), aStr,
			       KMsgBox::EXCLAMATION, 
			       i18n("Continue"), i18n("Cancel"));
      if(ret == 2) return false;
    }
  }

  status = doEncSign(persons, sign);

  // check for bad passphrase
  while(status & KpgpBase::BADPHRASE)
  {
    havePassPhrase = false;
    ret = KMsgBox::yesNoCancel(0,i18n("PGP Warning"),
			       i18n("You just entered an invalid passphrase.\n"
				    "Do you wan't to try again, continue and\n" 
				    "leave the message unsigned, "
				    "or cancel sending the message?"),
			       KMsgBox::EXCLAMATION, 
			       i18n("Retry"), i18n("Continue"));
    if(ret == 3) return false;
    if(ret == 2) sign = false;
    // ok let's try once again...
    status = doEncSign(persons, sign);
  }
  // check for bad keys
  if( status & KpgpBase::BADKEYS)
  {
    aStr = pgp->lastErrorMessage();
    aStr += "\n";
    aStr += i18n("Do you wan't to encrypt anyway, leave the\n" 
		 "message as is, or cancel the message?");
    ret = KMsgBox::yesNoCancel(0,i18n("PGP Warning"),
			      aStr, KMsgBox::EXCLAMATION,
			      i18n("Encrypt"), i18n("Continue"));
    if(ret == 3) return false;
    if(ret == 2) 
    {
      pgp->clearOutput();
      return true;
    }
    if(ret == 1) status = doEncSign(persons, sign, true);
  }
  if( status & KpgpBase::MISSINGKEY)
  {
    aStr = pgp->lastErrorMessage();
    aStr += "\n";
    aStr += i18n("Do you wan't to leave the message as is,\n" 
		 "or cancel the message?");
    ret = KMsgBox::yesNo(0,i18n("PGP Warning"),
			      aStr, KMsgBox::EXCLAMATION,
			      i18n("Continue"), i18n("Cancel"));
    if(ret == 2) return false;
    pgp->clearOutput();
    return true;
  }
  if( !(status & KpgpBase::ERROR) ) return true;

  // in case of other errors we end up here.
  errMsg = pgp->lastErrorMessage();
  return false;
  
}

int 
Kpgp::doEncSign(QStrList persons, bool sign, bool ignoreUntrusted)
{
  int retval;

  // to avoid error messages in case pgp is not installed
  if(!havePgp)
    return KpgpBase::OK;

  if(sign)
  {
    if(!prepare(TRUE)) return KpgpBase::ERROR;
    retval = pgp->encsign(&persons, passphrase, ignoreUntrusted);
  }  
  else
  {
    if(!prepare(FALSE)) return KpgpBase::ERROR;
    retval = pgp->encrypt(&persons, ignoreUntrusted);
  }
  // erase the passphrase if we do not want to keep it
  cleanupPass();

  return retval;
}

bool 
Kpgp::signKey(QString _key)
{
  if (!prepare(TRUE)) return FALSE;
  if(pgp->signKey(_key, passphrase) & KpgpBase::ERROR)
  {
    errMsg = pgp->lastErrorMessage();
    return false;
  }
  return true;
}


const QStrList*
Kpgp::keys(void)
{
  if (!prepare()) return NULL;

  if(publicKeys.isEmpty()) publicKeys = pgp->pubKeys();
  return &publicKeys;
}

bool 
Kpgp::havePublicKey(QString _person)
{
  if(!havePgp) return true;

  // do the checking case insensitive
  QString str;
  _person = _person.lower();
  _person = canonicalAdress(_person);

  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
  {
    str = str.lower();
    if(str.contains(_person)) return TRUE;
  }

  // reread the database, if key wasn't found...
  publicKeys = pgp->pubKeys();
  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
  {
    str = str.lower();
    if(str.contains(_person)) return TRUE;
  }

  return FALSE;
}

QString 
Kpgp::getPublicKey(QString _person)
{
  // just to avoid some error messages
  if(!havePgp) return true;

  // do the search case insensitive, but return the correct key.
  QString adress,str;
  adress = _person.lower();
  
  // first try the canonical mail adress.
  adress = canonicalAdress(adress);
  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
    if(str.contains(adress)) return str;

  // now try the pure input
  adress = _person.lower();
  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
    if(str.contains(adress)) return str;
  
  // FIXME: let user set the key/ get from keyserver

  return 0;
}

QString 
Kpgp::getAsciiPublicKey(QString _person)
{
  return pgp->getAsciiPublicKey(_person);
}

bool 
Kpgp::isEncrypted(void) const
{
  return pgp->isEncrypted();
}

const QStrList*
Kpgp::receivers(void) const
{
  return pgp->receivers();
}

const QString 
Kpgp::KeyToDecrypt(void) const
{
  //FIXME
  return 0;
}

bool
Kpgp::isSigned(void) const
{
  return pgp->isSigned();
}

QString 
Kpgp::signedBy(void) const
{
  return pgp->signedBy();
}

QString 
Kpgp::signedByKey(void) const
{
  return pgp->signedByKey();
}

bool 
Kpgp::goodSignature(void) const
{
  return pgp->isSigGood();
}

void 
Kpgp::setPassPhrase(const QString aPass)
{
  if (!aPass.isEmpty())
  {
    passphrase = aPass;
    havePassPhrase = TRUE;
  }
  else
  {
    if (!passphrase.isEmpty())
      passphrase.replace(QRegExp(".")," ");
    passphrase = 0;
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
  if(erasePassPhrase && havePassPhrase && !passphrase.isEmpty())
  {
    passphrase.replace(QRegExp(".")," ");
    passphrase = 0;
  }
  front = 0;
  back = 0;
}

const QString 
Kpgp::lastErrorMsg(void) const
{
  return errMsg;
}

bool 
Kpgp::havePGP(void) const
{
  return havePgp;
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

KSimpleConfig *
Kpgp::getConfig()
{
  return getKpgp()->config;
}


const QString
Kpgp::askForPass(QWidget *parent)
{
  return KpgpPass::getPassphrase(parent);
}

// --------------------- private functions -------------------

// check if pgp 2.6.x or 5.0 is installed
// kpgp will prefer to user pgp 5.0
bool 
Kpgp::checkForPGP(void)
{
  // get path
  QString path;
  QStrList pSearchPaths;
  int index = 0;
  int lastindex = -1;

  havePgp=FALSE;

  path = getenv("PATH");
  while((index = path.find(":",lastindex+1)) != -1)
  {
    pSearchPaths.append(path.mid(lastindex+1,index-lastindex-1));
    lastindex = index;
  }
  if(lastindex != (int)path.size() - 2)
    pSearchPaths.append( path.mid(lastindex+1,path.size()-lastindex-1) );

  QStrListIterator it(pSearchPaths);

  // first search for pgp5.0
  while ( it.current() )
  {
    path = it.current();
    path += "/pgpe";
    if ( !access( path, X_OK ) )
    {
      havePgp=TRUE;
      havePGP5=TRUE;
      debug("Kpgp: found pgp5.0");
      return TRUE;
    }
    ++it;
  }

  // lets try pgp2.6.x
  it.toFirst();
  while ( it.current() )
  {
       path = it.current();
       path += "/pgp";
       if ( !access( path, X_OK ) )
       {
	    havePgp=TRUE;
	    havePGP5=FALSE;
	    debug("Kpgp: found pgp2.6.x");
	    return TRUE;
       }
       ++it;
  }
  
  debug("Kpgp: no pgp found");
  return FALSE;
}

QString 
Kpgp::canonicalAdress(QString _adress)
{
  int index,index2;

  _adress = _adress.simplifyWhiteSpace();
  _adress = _adress.stripWhiteSpace();

  // just leave pure e-mail adress.
  if((index = _adress.find("<")) != -1)
    if((index2 = _adress.find("@",index+1)) != -1)
      if((index2 = _adress.find(">",index2+1)) != -1)
	return _adress.mid(index,index2-index);
  
  if((index = _adress.find("@")) == -1)
  { 
    // local adress
    char* hostname = (char *)malloc(1024*sizeof(char));
    gethostname(hostname,1024);
    QString adress = '<';
    adress += _adress;
    adress += '@';
    adress += hostname;
    adress += '>';
    return adress;
  }
  else
  {
    int index1 = _adress.findRev(" ",index);
    int index2 = _adress.find(" ",index);
    if(index2 == -1) index2 = _adress.length();
    QString adress = "<";
    adress += _adress.mid(index1+1 ,index2-index1-1);
    adress += ">";
    return adress;
  }
}


//----------------------------------------------------------------------
//  widgets needed by kpgp
//----------------------------------------------------------------------

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

KpgpKey::KpgpKey(QWidget *parent, const char *name, QStrList *keys)
  : QDialog(parent, 0, TRUE)
{
  KIconLoader* loader = kapp->getIconLoader();
  QPixmap pixm;

  setCaption(name);
  setFixedSize(350,120);
  cursor = kapp->overrideCursor();
  if(cursor != 0)
    kapp->setOverrideCursor(QCursor(ibeamCursor));
  this->setCursor(QCursor(ibeamCursor));
  QLabel *text = new QLabel(i18n("Please select the public key to insert"),this);
  text->move(56,4);
  text->setAutoResize(TRUE);
  QLabel *icon = new QLabel(this);
  pixm = loader->loadIcon("pgp-keys.xpm");
  icon->setPixmap(pixm);
  icon->move(4,8);
  icon->resize(48,48);

  combobox = new QComboBox(FALSE, this, "combo");
  combobox->insertStrList(keys);
  combobox->setFocus();

  button = new QButton(i18n("&Insert"),this);

  connect(button,SIGNAL(clicked()),this,SLOT(accept()) );  
}

KpgpKey::~KpgpKey()
{
  if(cursor != 0)
    kapp->restoreOverrideCursor();
}

QString 
KpgpKey::getKeyName(QWidget *parent, QStrList *keys)
{
  KpgpKey pgpkey(parent, i18n("Select key"));
  kpgppass.exec();
  return kpgppass.getKey().copy();
}

QString
KpgpPass::getKey()
{
  return combobox->currentText();
}


// ------------------------------------------------------------------------

KpgpConfig::KpgpConfig(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
  pgp = Kpgp::getKpgp();

  QBoxLayout* box = new QBoxLayout(this, QBoxLayout::TopToBottom, 3);

  QGroupBox *grp = new QGroupBox(i18n("Identity"), this);
  box->addWidget(grp);

  QGridLayout* grid = new QGridLayout(grp, 1, 2, 20, 6);

  pgpUserEdit = createLabeledEntry(grid, 
				   i18n("PGP User Identity:"),
				   pgp->user(), 0, 0);

  grid->setColStretch(0,1);
  grid->setColStretch(1,10);
  grid->activate();

  QGroupBox *grp2 = new QGroupBox(i18n("Options"), this);
  QGridLayout* grid2 = new QGridLayout(grp2, 2, 3, 20, 6);

  storePass=new QCheckBox(
	      i18n("Store passphrase"), grp2);
  storePass->adjustSize();
  storePass->setMinimumSize(storePass->sizeHint());
  grid2->addMultiCellWidget(storePass, 0, 0, 0, 2);

  encToSelf=new QCheckBox(
	      i18n("Always encrypt to self"), grp2);
  encToSelf->adjustSize();
  encToSelf->setMinimumSize(encToSelf->sizeHint());
  grid2->addMultiCellWidget(encToSelf, 1, 1, 0, 2);

  grid2->setColStretch(0,0);
  grid2->setColStretch(1,1);
  grid2->setColStretch(2,0);
  grid2->activate();

  box->addWidget(grp2);

  box->addStretch(10);
  box->activate();

  // set default values
  pgpUserEdit->setText(pgp->user());
  storePass->setChecked(pgp->storePassPhrase()); 
  encToSelf->setChecked(pgp->encryptToSelf());

}

KpgpConfig::~KpgpConfig()
{
}

void 
KpgpConfig::applySettings()
{
  pgp->setUser(pgpUserEdit->text());
  pgp->setStorePassPhrase(storePass->isChecked());
  pgp->setEncryptToSelf(encToSelf->isChecked());

  pgp->writeConfig(true);
}

QLineEdit* 
KpgpConfig::createLabeledEntry(QGridLayout* grid,
			       const char* aLabel,
			       const char* aText, 
			       int gridy, int gridx)
{
  QLabel* label = new QLabel(this);
  QLineEdit* edit = new QLineEdit(this);

  label->setText(aLabel);
  label->adjustSize();
  label->resize((int)label->sizeHint().width(),label->sizeHint().height() + 6);
  label->setMinimumSize(label->size());
  grid->addWidget(label, gridy, gridx++);

  if (aText) edit->setText(aText);
  edit->setMinimumSize(100, label->height()+2);
  edit->setMaximumSize(1000, label->height()+2);
  grid->addWidget(edit, gridy, gridx++);

  return edit;
}


