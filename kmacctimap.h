/**
 * kmacctimap.h
 *
 * Copyright (c) 2000 Michael Haeckel <Michael@Haeckel.Net>
 *
 * This file is based on kmacctexppop.h by Don Sanders
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef KMAcctImap_h
#define KMAcctImap_h

#include "kmaccount.h"

#define KMAcctImapInherited KMAccount

//-----------------------------------------------------------------------------
class KMAcctImap: public KMAccount
{
  Q_OBJECT

public:
  virtual ~KMAcctImap();
  virtual void init(void);

  /**
   * Imap user login name
   */
  const QString& login(void) const { return mLogin; }
  virtual void setLogin(const QString&);

  /**
   * Imap user password
   */
  QString passwd(void) const;
  virtual void setPasswd(const QString&, bool storeInConfig=FALSE);

  /**
   * Imap authentificaion method
   */
  QString auth(void) const { return mAuth; }
  virtual void setAuth(const QString&);

  /**
   * Will the password be stored in the config file ?
   */
  bool storePasswd(void) const { return mStorePasswd; }
  virtual void setStorePasswd(bool);

  /**
   * Imap host
   */
  const QString& host(void) const { return mHost; }
  virtual void setHost(const QString&);

  /**
   * Port on Imap host
   */
  unsigned short int port(void) { return mPort; }
  virtual void setPort(unsigned short int);

  /**
   * Prefix to the Imap folders
   */
  const QString& prefix(void) const { return mPrefix; }
  virtual void setPrefix(const QString&);

  /**
   * Automatically expunge deleted messages when leaving the folder
   */
  bool autoExpunge() { return mAutoExpunge; }
  virtual void setAutoExpunge(bool);

  /**
   * Show hidden files on the server
   */
  bool hiddenFolders() { return mHiddenFolders; }
  virtual void setHiddenFolders(bool);

  /**
   * Use SSL or not
   */
  bool useSSL() { return mUseSSL; }
  virtual void setUseSSL(bool);

  /**
   * Use TLS or not
   */
  bool useTLS() { return mUseTLS; }
  virtual void setUseTLS(bool);

  /**
   * Inherited methods.
   */
  virtual const char* type(void) const;
  virtual void readConfig(KConfig&);
  virtual void writeConfig(KConfig&);
  virtual void processNewMail(bool) { emit finishedCheck(false); }
  virtual void pseudoAssign(KMAccount*);

protected:
  friend class KMAcctMgr;
  friend class KMPasswdDialog;
  KMAcctImap(KMAcctMgr* owner, const QString& accountName);

  /**
   * Very primitive en/de-cryption so that the password is not
   * readable in the config file. But still very easy breakable.
   */
  QString encryptStr(const QString &inStr) const;
  QString decryptStr(const QString &inStr) const;

  QString mLogin, mPasswd;
  QString mHost, mAuth;
  QString mPrefix;
  unsigned short int mPort;
  bool    mStorePasswd;
  bool    mAutoExpunge;
  bool    mUseSSL;
  bool    mUseTLS;
  bool    mHiddenFolders;
};

#endif /*KMAcctImap_h*/
