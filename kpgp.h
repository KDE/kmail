/* KPGP: Pretty good privacy en-/decryption class
 * This code is under GPL V2.0
 */
#ifndef KPGP_H
#define KPGP_H

#include <stdio.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qobject.h>
#include <qlabel.h>
#include <qlined.h>

#include <kprocess.h>
#include <kapp.h>

#define ENCRYPT 1
#define SIGN    2
#define ENCSIGN 3
#define DECRYPT 4
#define TEST    0

class Kpgp : public QObject
{
  Q_OBJECT
     
public:
  Kpgp();
  ~Kpgp();

  void readConfig();
  void writeConfig(bool sync);

  /** sets the message to en- or decrypt 
      returns TRUE if the message contains any pgp encoded or signed
      parts */
  bool setMessage(const QString mess);
  /** gets the de (or en)crypted message */
  QString getMessage();

  /** decrypts the message if the passphrase is good.
      returns false otherwise */
  bool decrypt(QString *passphrase = 0);
  /** encrypt message to a list of persons */
  bool encryptTo(QStrList *pers, bool sign = TRUE, QString *pass = 0);
  /** sign a message */
  bool sign(QString *pass = 0);

  /** is the message encrypted ? */
  bool isEncrypted();
  /** the persons who can decrypt the message */
  QStrList isEncryptedFor();
  /** shows the secret key which is needed
      to decrypt the message */
  QString KeyToDecrypt();
     
  /** is the message signed by someone */
  bool isSigned();
  /** it is signed by ... */
  QString signedBy();
  /** keyID of signer */
  QString signedByKey();
  /** is the signature good ? */
  bool goodSignature();

  /** always encrypt message to oneself? */
  void setEncryptToSelf(bool flag);
  bool encryptToSelf();


  /** store passphrase in pgp object
      Problem: passphrase bleibt im Speicher, macht das ganze also
      etwas unsicherer (evtl kodieren????). en-/decrypt() kann dann
      ohne passphrase aufgerufen werden. */
  bool storePassPhrase(void);
  void setStorePassPhrase(bool);

  void setPassPhrase(QString *pass);

  /** changing the passphrase of the actual secret key */
  bool changePassPhrase(QString *oldPassPhrase, QString
			*newPassPhrase);
  /** set a secret key to use (if you have more than one...)
    * defaultmaessig wird von pgp immer der zuletzt generierte
    * key benutzt. Muss also nicht unbedingt gesetzt werden */
  void setSecretKey(const QString secKey);
  /** returns the keyId of the actual secret key*/
  const QString secretKey(void);

  /** clears everything from memory */
  void clear(bool erasePassPhrase = FALSE);

  /** returns the last error that occured */
  const QString lastErrorMsg();

  /// could we find a pgp executable?
  bool havePGP();

  // FIXME: key management

  // static methods

  /** return the actual pgp object */
  static Kpgp *getKpgp();

  /** pops up a window which asks for the passphrase */
  static QString *askForPass();

  /** does the whole decryption in one step. Pops up a message box
      if it needs to ask for a passphrase */ 
  static QString decode(const QString text, bool returnHTML=FALSE);

public slots:
  void runPGPInfo(KProcess *, char *buf, int buflen);
  void runPGPOut(KProcess *, char *buf, int buflen);
  void runPGPfinished(KProcess *);
  void runPGPcloseStdin(KProcess *);

private:
  bool checkForPGP();
  bool runPGP(const QString *pass = 0, int action = TEST, 
	      QStrList *args = 0);
  bool parseInfo(int action);

  KConfig *config;
  static Kpgp *kpgpObject;

  QString message;
  QString signature;
  QString signatureID;
  QString keyNeeded;
  QString secKey;
  QStrList *persons;
  QString info;
  QString output;
  QString errMsg;

  bool flagNoPGP;
  bool flagEncrypted;
  bool flagSigned;
  bool flagSigIsGood;
  bool flagEncryptToSelf;
  bool havePassPhrase;
  bool pgpNotFinished;

  bool storePass;
  QString *passPhrase;
};

class KpgpPass : public QWidget
{
  Q_OBJECT

public:
  KpgpPass();
  ~KpgpPass();

  static QString *getPassphrase();

public slots:
  void passphraseEntered();

private:
  QString pass;

  QLabel *text;
  QLineEdit *lineedit;
  bool gotPassphrase;
};

#endif
