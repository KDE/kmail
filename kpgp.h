/** KPGP: Pretty good privacy en-/decryption class
 *        This code is under GPL V2.0
 *
 * @author Lars Knoll <knoll@mpi-hd.mpg.de>
 *
 * GNUPG support
 * @author "J. Nick Koston" <bdraco@the.system.is.halted.net> 
 *
 * PGP6 and other enhancements
 * @author Andreas Gungl <Andreas.Gungl@osp-dd.de>
 */
#ifndef KPGP_H
#define KPGP_H

#include <stdio.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbt.h>
#include <qlistbox.h>

class QLineEdit;
class QCursor;
class QCheckBox;
class QGridLayout;

class KSimpleConfig;
class KpgpBase;

class Kpgp
{

private:
  // the class running pgp
  KpgpBase *pgp;

public:
  Kpgp();
  virtual ~Kpgp();

  virtual void readConfig();
  virtual void writeConfig(bool sync);
  virtual void init();

  /** sets the message to en- or decrypt 
    returns TRUE if the message contains any pgp encoded or signed
    parts 
    The format is then:
    frontmatter()
    ----- BEGIN PGP ...
    message()
    ----- END PGP ...
    backmatter()
    */
  virtual bool setMessage(const QString mess);
  /** gets the de- (or en)crypted message */
  virtual const QString message(void) const;
  /** gets the part before the decrypted message */
  virtual const QString frontmatter(void) const;
  /** gets the part after the decrypted message */
  virtual const QString backmatter(void) const;

  /** decrypts the message if the passphrase is good.
    returns false otherwise */
  bool decrypt(void);
  /** encrypt the message for a list of persons. */
  bool encryptFor(const QStrList& receivers, bool sign = TRUE);

protected:
  int doEncSign(QStrList persons, bool sign, bool ignoreUntrusted = false);

public:
  /** sign the message. */
  bool sign(void);
  /** sign a key in the keyring with users signature. */
  bool signKey(QString _key);
  /** get the known public keys. */
  const QStrList* keys(void);
  /** check if we have a public key for given person. */
  bool havePublicKey(QString person);
  /** try to get the public key for this person */
  QString getPublicKey(QString _person);
  /** try to ascii output of the public key of this person */
  QString getAsciiPublicKey(QString _person);
     
  /** is the message encrypted ? */
  bool isEncrypted(void) const;
  /** the persons who can decrypt the message */
  const QStrList* receivers(void) const;
  /** shows the secret key which is needed
    to decrypt the message */
  const QString KeyToDecrypt(void) const;

  /** is the message signed by someone */
  bool isSigned(void) const;
  /** it is signed by ... */
  QString signedBy(void) const;
  /** keyID of signer */
  QString signedByKey(void) const;
  /** is the signature good ? */
  bool goodSignature(void) const;

  /** change the passphrase of the actual secret key */
  bool changePassPhrase(const QString oldPass, const QString newPass);

  /** Set pass phrase */
  void setPassPhrase(const QString pass);

 /** set a user identity to use (if you have more than one...)
   * by default, pgp uses the identity which was generated last. */
  void setUser(const QString user);
  /** Returns the actual user identity. */
  const QString user(void) const;

  /** always encrypt message to oneself? */
  void setEncryptToSelf(bool flag);
  bool encryptToSelf(void) const;

  /** store passphrase in pgp object
    Problem: passphrase stays in memory. 
    Advantage: you can call en-/decrypt without always passing the
    passphrase */
  void setStorePassPhrase(bool);
  bool storePassPhrase(void) const;

  /** clears everything from memory */
  void clear(bool erasePassPhrase = FALSE);

  /** returns the last error that occured */
  const QString lastErrorMsg(void) const;

  /// did we find a pgp executable?
  bool havePGP(void) const;

  // FIXME: key management

  // static methods

  /** return the actual pgp object */
  static Kpgp *getKpgp();

  /** get the kpgp config object */
  static KSimpleConfig *getConfig();

  /** pops up a modal window which asks for the passphrase 
   puts the window on top of the parent, or in the middle of the screen,
   if parent = 0 */
  static const QString askForPass(QWidget *parent = 0);

private:
  // test if the PGP executable is found and if there is a passphrase
  // set or given. Returns TRUE if everything is ok, and FALSE (together
  // with some warning message) if something is missing.
  bool prepare(bool needPassPhrase=FALSE);

  // cleanup passphrase if it should not be stored.
  void cleanupPass(void);

  // transform an adress into canonical form
  QString canonicalAdress(QString _person);

  //Select public key from a list of all public keys
  QString SelectPublicKey(QStrList pbkeys, const char *caption);

  bool checkForPGP(void);

  static Kpgp *kpgpObject;
  KSimpleConfig *config;

  QStrList publicKeys;
  QString front;
  QString back;
  QString errMsg;

  bool storePass;
  QString passphrase;

  bool havePgp;
  bool havePGP5;
  bool haveGpg;
  bool havePassPhrase;
  bool needPublicKeys;
};

// -------------------------------------------------------------------------
class KpgpPass : public QDialog
{
  Q_OBJECT

public:
  /** the passphrase dialog */
  KpgpPass(QWidget *parent = 0, const char *name = 0);
  virtual ~KpgpPass();

  static QString getPassphrase(QWidget *parent = 0);

private:
  QString getPhrase();

  QLineEdit *lineedit; 
  QCursor *cursor;
};

// -------------------------------------------------------------------------
class KpgpKey : public QDialog
{
  Q_OBJECT

public:
  /** the passphrase dialog */
  KpgpKey(QWidget *parent = 0, const char *name = 0, const QStrList *keys = NULL);
  virtual ~KpgpKey();

  static QString getKeyName(QWidget *parent = 0, const QStrList *keys = NULL);

private:
  QString getKey();

  QComboBox *combobox; 
  QPushButton *button;
  QCursor *cursor;
};

// -------------------------------------------------------------------------
class KpgpConfig : public QWidget
{
  Q_OBJECT

public:
  /** a widget for configuring the pgp interface. Can be included into
   a tabdialog. This widget by itself does not provide an apply/cancel
   button mechanism. */
  KpgpConfig(QWidget *parent = 0, const char *name = 0);
  virtual ~KpgpConfig();

  virtual void applySettings();
       
protected:
  Kpgp *pgp;
  QLineEdit *pgpUserEdit;
  QCheckBox *storePass;
  QCheckBox *encToSelf;

};
 
// -------------------------------------------------------------------------
class KpgpSelDlg: public QDialog
{
  Q_OBJECT
public:
  KpgpSelDlg(QStrList aKeyList, const char *caption=NULL);
  virtual ~KpgpSelDlg() {};

  virtual const QString key(void) const {return mkey;};

protected slots:
  void slotOk();
  void slotCancel();

protected:
  QGridLayout mGrid;
  QListBox mListBox;
  QPushButton mBtnOk, mBtnCancel;
  QString mkey;
  QStrList mKeyList;
};

#endif

