/** KPGP: Pretty good privacy en-/decryption class
 *        This code is under GPL V2.0
 *
 * @author Lars Knoll <knoll@mpi-hd.mpg.de>
 */
#ifndef KPGP_H
#define KPGP_H

#include <stdio.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qlined.h>
#include <qdialog.h>
#include <qcursor.h>

class Kpgp: public QObject
{
  Q_OBJECT

  enum Action {
    TEST    = 0,
    ENCRYPT = 1,
    SIGN    = 2,
    ENCSIGN = 3,
    DECRYPT = 4,
    PUBKEYS = 5,
    SIGNKEY = 6
  };
     
public:
  Kpgp();
  ~Kpgp();

  void readConfig();
  void writeConfig(bool sync);

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
  bool setMessage(const QString mess);
  /** gets the de- (or en)crypted message */
  const QString message(void) const;
  /** gets the part before the decrypted message */
  const QString frontmatter(void) const;
  /** gets the part after the decrypted message */
  const QString backmatter(void) const;

  /** decrypts the message if the passphrase is good.
    returns false otherwise */
  bool decrypt(void);
  /** encrypt the message for a list of persons. */
  bool encryptFor(const QStrList& receivers, bool sign = TRUE);
  /** sign the message. */
  bool sign(void);
  /** sign a key in the keyring with users signature. */
  bool signKey(QString _key);
  /** get the known public keys. */
  const QStrList* keys(void);
  /** check if we have a public key for given person. */
  bool havePublicKey(QString person);
     
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

  /** always encrypt message to oneself? */
  void setEncryptToSelf(bool flag);
  bool encryptToSelf(void) const;

  /** store passphrase in pgp object
    Problem: passphrase stays in memory. 
    Advantage: you can call en-/decrypt without always passing the
    passphrase */
  bool storePassPhrase(void) const;
  void setStorePassPhrase(bool);

  /** Set pass phrase */
  void setPassPhrase(const QString pass);

  /** change the passphrase of the actual secret key */
  bool changePassPhrase(const QString oldPass, const QString newPass);

  /** set a user identity to use (if you have more than one...)
   * by default, pgp uses the identity which was generated last. */
  void setUser(const QString user);

  /** Returns the actual user identity. */
  const QString user(void) const { return pgpUser; }

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

  /** pops up a modal window which asks for the passphrase 
   puts the window on top of the parent, or in the middle of the screen,
   if parent = 0 */
  static const QString askForPass(QWidget *parent = 0);

  /** does the whole decryption in one step. Pops up a message box
    if it needs to ask for a passphrase */ 
  static const QString decode(const QString text, bool returnHTML = FALSE);

private:
  // test if the PGP executable is found and if there is a passphrase
  // set or given. Returns TRUE if everything is ok, and FALSE (together
  // with some warning message) if something is missing.
  bool prepare(bool needPassPhrase=FALSE);

  // cleanup passphrase if it should not be stored.
  void cleanupPass(void);

  bool checkForPGP(void);
  bool runPGP(int action = TEST, const char *args = 0);
  bool parseInfo(int action);

  static Kpgp *kpgpObject;

  QString input;
  QString signature;
  QString signatureID;
  QString keyNeeded;
  QString pgpUser; // was: secKey
  QStrList persons;
  QStrList publicKeys;
  QString info;
  QString output;
  QString front;
  QString back;
  QString errMsg;

  bool flagNoPGP;
  bool flagEncrypted;
  bool flagSigned;
  bool flagSigIsGood;
  bool flagEncryptToSelf;
  bool havePassPhrase;
  bool pgpRunning;

  bool storePass;
  QString passPhrase;
};

class KpgpPass : public QDialog
{
  Q_OBJECT

public:
  KpgpPass(QWidget *parent = 0, const char *name = 0);
  ~KpgpPass();

  static QString getPassphrase(QWidget *parent = 0);

private:
  QString getPhrase();

  QLineEdit *lineedit; 
  QCursor *cursor;
};

#endif
