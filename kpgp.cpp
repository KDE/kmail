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
#include <qhbox.h> 
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include <kapp.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>


Kpgp *Kpgp::kpgpObject = 0L;

Kpgp::Kpgp()
  : publicKeys()
{
  kpgpObject=this;

  config = new KSimpleConfig("kpgprc" );

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
  passphrase = QString::null;

  // do we have a pgp executable
  checkForPGP();

  if(havePgp)
  {
    if(havePGP5)
      pgp = new KpgpBase5();
    else if (haveGpg)
      pgp = new KpgpBaseG();
    else 
    {
      KpgpBase6 *pgp_v6 = new KpgpBase6();
      if (!pgp_v6->isVersion6())
      {
        delete pgp_v6;
        pgp = new KpgpBase2();
      }
      else pgp = pgp_v6;
    }
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
  // No! This takes time since pgp takes some ridicules
  // time to start, blocking everything (pressing any key _on_
  // _the_ _machine_ _where_ _pgp_ _runs: helps; ???)
  // So we will ask for keys when we need them.
  //publicKeys = pgp->pubKeys(); This can return 0!!!

  needPublicKeys = true;
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
    back  = mess.right(mess.length() - index);

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
    passphrase = QString::null;
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
      int ret = KMessageBox::warningContinueCancel(0,
			       i18n("Could not find the public keys for the\n"
				    "recipients of this mail.\n"
				    "Message will not be encrypted."),
                               i18n("PGP Warning"), i18n("C&ontinue"));
      if(ret == KMessageBox::Cancel) return false;
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
	aStr += i18n("These persons will not be able to decrypt the message\n");
      else
	aStr += i18n("This person will not be able to decrypt the message\n");

      int ret = KMessageBox::warningContinueCancel(0, aStr, 
                               i18n("PGP Warning"), 
			       i18n("C&ontinue"));
      if(ret == KMessageBox::Cancel) return false;
    }
  }

  status = doEncSign(persons, sign);

  // check for bad passphrase
  while(status & KpgpBase::BADPHRASE)
  {
    havePassPhrase = false;
    ret = KMessageBox::warningYesNoCancel(0,
				   i18n("You just entered an invalid passphrase.\n"
					"Do you wan't to try again, continue and\n"
					"leave the message unsigned, "
					"or cancel sending the message?"),
                                   i18n("PGP Warning"),
				   i18n("&Retry"), i18n("Send &unsigned"));
    if(ret == KMessageBox::Cancel) return false;
    if(ret == KMessageBox::No) sign = false;
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
    ret = KMessageBox::warningYesNoCancel(0,aStr, i18n("PGP Warning"),
                           i18n("&Encrypt"), i18n("&Send unecrypted"));
    if(ret == KMessageBox::Cancel) return false;
    if(ret == KMessageBox::No)
    {
      pgp->clearOutput();
      return true;
    }
    if(ret == KMessageBox::Yes) status = doEncSign(persons, sign, true);
  }
  if( status & KpgpBase::MISSINGKEY)
  {
    aStr = pgp->lastErrorMessage();
    aStr += "\n";
    aStr += i18n("Do you want to leave the message as is,\n"
		 "or cancel the message?");
    ret = KMessageBox::warningContinueCancel(0,aStr, i18n("PGP Warning"),
				   i18n("&Send as is"));
    if(ret == KMessageBox::Cancel) return false;
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
  if (needPublicKeys)
  {
    publicKeys = pgp->pubKeys();
    needPublicKeys=false;
  }

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
  if(!havePgp) return QString::null;
  if (needPublicKeys)
  {
    publicKeys = pgp->pubKeys();
    needPublicKeys=false;
  }

  // do the search case insensitive, but return the correct key.
  QString adress,str,disp_str;
  adress = _person.lower();

  // first try the canonical mail adress.
  adress = canonicalAdress(adress);
  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
    if(str.contains(adress)) return str;

  // now check if the key contains the address
  adress = _person.lower();
  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
    if(str.contains(adress)) return str;

  // reread the database, because key wasn't found...
  publicKeys = pgp->pubKeys();

  // FIXME: let user set the key/ get from keyserver
  // now check if the address contains the key
  adress = _person.lower();
  for(str=publicKeys.first(); str!=0; str=publicKeys.next())
    if(adress.contains(str)) return str;

  // no match until now, let the user choose the key:
  adress = canonicalAdress(adress);
  str= SelectPublicKey(publicKeys, adress);
  if (!str.isEmpty()) return str;

  return QString::null;
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
  return QString::null;
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
    passphrase = QString::null;
    havePassPhrase = FALSE;
  }
}

bool
Kpgp::changePassPhrase(const QString /*oldPass*/,
		       const QString /*newPass*/)
{
  //FIXME...
  KMessageBox::information(0,i18n("Sorry, but this feature\nis still missing"));
  return FALSE;
}

void
Kpgp::clear(bool erasePassPhrase)
{
  if(erasePassPhrase && havePassPhrase && !passphrase.isEmpty())
  {
    passphrase.replace(QRegExp(".")," ");
    passphrase = QString::null;
  }
  front = QString::null;
  back = QString::null;
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
  if(lastindex != (int)path.length() - 1)
    pSearchPaths.append( path.mid(lastindex+1,path.length()-lastindex) );

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
      haveGpg=FALSE;
      return TRUE;
    }
    ++it;
  }

  // lets try gpg
  it.toFirst();
  while ( it.current() )
  {
    path = it.current();
    path += "/gpg";
    if ( !access( path, X_OK ) )
    {
      havePgp=TRUE;
      havePGP5=FALSE;
      haveGpg=TRUE;
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
	    haveGpg=FALSE;
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
    // local address
    char hostname[1024];
    gethostname(hostname,1024);
    QString adress = "<";
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

QString
Kpgp::SelectPublicKey(QStrList pbkeys, const char *caption)
{
  KpgpSelDlg dlg(pbkeys,caption);
  QString txt ="";

  if (dlg.exec()==QDialog::Rejected) return 0;
  txt = dlg.key();
  if (!txt.isEmpty())
  {
    return txt;
  }
  return 0;
}


//----------------------------------------------------------------------
//  widgets needed by kpgp
//----------------------------------------------------------------------

KpgpPass::KpgpPass(QWidget *parent, const QString &name, bool modal )
  :KDialogBase( parent, name, modal, i18n("OpenPGP Security Check"), 
                Ok|Cancel )
{
  QHBox *hbox = makeHBoxMainWidget();
  hbox->setSpacing( spacingHint() );
  hbox->setMargin( marginHint() );

  QLabel *label = new QLabel(hbox);
  label->setPixmap( BarIcon("pgp-keys") );

  QWidget *rightArea = new QWidget( hbox );
  QVBoxLayout *vlay = new QVBoxLayout( rightArea, 0, spacingHint() );
  
  label = new QLabel(i18n("Please enter your OpenPGP passphrase"),rightArea);
  lineedit = new QLineEdit( rightArea );
  lineedit->setEchoMode(QLineEdit::Password);
  lineedit->setMinimumWidth( fontMetrics().maxWidth()*20 );
  lineedit->setFocus();
  connect( lineedit, SIGNAL(returnPressed()), this, SLOT(slotOk()) );

  vlay->addWidget( label );
  vlay->addWidget( lineedit );

  disableResize();
}


KpgpPass::~KpgpPass()
{
}

QString
KpgpPass::getPassphrase(QWidget *parent)
{
  KpgpPass kpgppass(parent, i18n("OpenPGP Security Check"));
  if (kpgppass.exec())
    return kpgppass.getPhrase().copy();
  else
    return QString::null;
}

QString
KpgpPass::getPhrase()
{
  return lineedit->text();
}



// ------------------------------------------------------------------------

KpgpKey::KpgpKey( QStrList *keys, QWidget *parent, const char *name, 
		  bool modal ) 
  :KDialogBase( i18n("Select key"), Yes, Yes, Yes, parent, name, modal, 
		true, i18n("&Insert") )
{
  QHBox *hbox = new QHBox( this );
  setMainWidget( hbox );
  hbox->setSpacing( spacingHint() );
  hbox->setMargin( marginHint() );

  QLabel *label = new QLabel(hbox);
  label->setPixmap( BarIcon("pgp-keys") );

  QWidget *rightArea = new QWidget( hbox );
  QVBoxLayout *vlay = new QVBoxLayout( rightArea, 0, spacingHint() );
  
  label = new QLabel(i18n("Please select the public key to insert"),rightArea);
  combobox = new QComboBox( FALSE, rightArea, "combo" );
  combobox->setFocus();
  if( keys != 0 )
  {
    combobox->insertStrList(keys);
  }
  vlay->addWidget( label );
  vlay->addWidget( combobox );


  setCursor( QCursor(arrowCursor) );
  cursor = kapp->overrideCursor();
  if( cursor != 0 )
    kapp->setOverrideCursor( QCursor(arrowCursor) );

  disableResize();
}


KpgpKey::~KpgpKey()
{
  if(cursor != 0)
    kapp->restoreOverrideCursor();
}


QString
KpgpKey::getKeyName(QWidget *parent, const QStrList *keys)
{
  KpgpKey pgpkey( (QStrList*)keys, parent );
  pgpkey.exec();
  return pgpkey.getKey().copy();
}


QString
KpgpKey::getKey()
{
  return combobox->currentText();
}


// ------------------------------------------------------------------------
KpgpConfig::KpgpConfig(QWidget *parent, const char *name)
  : QWidget(parent, name), pgp( Kpgp::getKpgp() )
{
  QVBoxLayout *topLayout = new QVBoxLayout( this, 0, KDialog::spacingHint() );
  
  QGroupBox *group = new QGroupBox( i18n("Identity"), this );
  topLayout->addWidget( group );
  QGridLayout *glay = new QGridLayout( group, 2, 2,  KDialog::spacingHint() );
  glay->addRowSpacing( 0, fontMetrics().lineSpacing() );  

  QLabel *label = new QLabel( i18n("PGP User Identity:"), group );
  pgpUserEdit = new QLineEdit( group );
  pgpUserEdit->setText( pgp->user() );
  glay->addWidget( label, 1, 0 );
  glay->addWidget( pgpUserEdit, 1, 1 );

  group = new QGroupBox( i18n("Options"), this );
  topLayout->addWidget( group );
  QVBoxLayout *vlay = new QVBoxLayout( group, KDialog::spacingHint() );
  vlay->addSpacing( fontMetrics().lineSpacing() );  

  storePass = new QCheckBox( i18n("Keep passphrase in memory"), group );
  encToSelf = new QCheckBox( i18n("Always encrypt to self"), group );
  vlay->addWidget( storePass );
  vlay->addWidget( encToSelf );

  setValues();
}


KpgpConfig::~KpgpConfig()
{
}

void
KpgpConfig::setValues()
{
  // set default values
  pgpUserEdit->setText( pgp->user() );
  storePass->setChecked( pgp->storePassPhrase() );
  encToSelf->setChecked( pgp->encryptToSelf() );
}

void
KpgpConfig::applySettings()
{
  pgp->setUser(pgpUserEdit->text());
  pgp->setStorePassPhrase(storePass->isChecked());
  pgp->setEncryptToSelf(encToSelf->isChecked());

  pgp->writeConfig(true);
}



// ------------------------------------------------------------------------
KpgpSelDlg::KpgpSelDlg( const QStrList &aKeyList, const QString &recipent,
			QWidget *parent, const char *name, bool modal )
  :KDialogBase( parent, name, modal, i18n("PGP Key Selection"), Ok|Cancel, Ok)
{
  QFrame *page = makeMainWidget();
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  
  QLabel *label = new QLabel( page );
  label->setText(i18n("Select public key for recipient \"%1\"").arg(recipent));
  topLayout->addWidget( label );

  mListBox = new QListBox( page );
  mListBox->setMinimumHeight( fontMetrics().lineSpacing() * 7 );
  mListBox->setMinimumWidth( fontMetrics().maxWidth() * 25 );
  topLayout->addWidget( mListBox, 10 );

  mKeyList = aKeyList;
  mkey = "";

  for( const char *key = mKeyList.first(); key!=0; key = mKeyList.next() )
  {
    //insert only real keys:
    //if (!(QString)key.contains("matching key"))
    mListBox->insertItem(key);
  }
  if( mListBox->count() > 0 )
  {
    mListBox->setCurrentItem(0);
  } 
}


void KpgpSelDlg::slotOk()
{
  int idx = mListBox->currentItem();
  if (idx>=0) mkey = mListBox->text(idx);
  else mkey = "";

  accept();
}


void KpgpSelDlg::slotCancel()
{
  mkey = "";
  reject();
}
