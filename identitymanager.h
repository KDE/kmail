/*  -*- c++ -*-
    identitymanager.h

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/


#ifndef _KMAIL_IDENTITYMANAGER_H_
#define _KMAIL_IDENTITYMANAGER_H_

#include "configmanager.h"

#include <qvaluelist.h>

class KMKernel;
class QStringList;
class KMIdentity;

/**
 * @short Manages the list of identities.
 * @author Marc Mutz <mutz@kde.org>
 **/
class IdentityManager : public ConfigManager {
  Q_OBJECT
#ifndef KMAIL_TESTING
protected:
  friend class KMKernel;
#else
public:
#endif // KMAIL_TESTING

  IdentityManager( QObject * parent=0, const char * name=0 );
  virtual ~IdentityManager();

public:
  typedef QValueList<KMIdentity>::Iterator Iterator;
  typedef QValueList<KMIdentity>::ConstIterator ConstIterator;

  /** Commit changes to disk and emit changed() if necessary. */
  void commit();
  /** Re-read the config from disk and forget changes. */
  void rollback();

  /** Check whether there are any unsaved changes. */
  bool hasPendingChanges() const;

  /** @return the list of identities */
  QStringList identities() const;

  /** Convenience method.

      @return the list of (shadow) identities, ie. the ones currently
      under configuration.
  */
  QStringList shadowIdentities() const;

  /** Sort the identities by name (the default is always first). This
      operates on the @em shadow list, so you need to @ref commit for
      the changes to take effect.
  **/
  void sort();

  /** @return an identity whose address matches any in @p addressList
      or @ref KMIdentity::null if no such identity exists.
  **/
  const KMIdentity & identityForAddress( const QString & addressList ) const;

  /** @return the identity named @p identityName or @ref
      KMIdentity::null if not found.
  **/
  const KMIdentity & identityForName( const QString & identityName ) const;

  /** Convenience method.
  
      @return the identity named @p identityName or the default
      identity if not found.
  **/
  const KMIdentity & identityForNameOrDefault( const QString & identityName ) const;

  /** @return the default identity */
  const KMIdentity & defaultIdentity() const;

  /** Sets the identity named @p identityName to be the new default
      identity. As usual, use @ref commit to make this permanent.
      
      @return false if an identity named @p identityName was not found
  **/
  bool setAsDefault( const QString & identityName );

  /** @return the identity named @p idenityName. This method returns a
      reference to the identity that can be modified. To let others
      see this change, use @ref commit.
  **/
  KMIdentity & identityForName( const QString & identityName );

  /** Removes the identity with name @p identityName */
  bool removeIdentity( const QString & identityName );

  ConstIterator begin() const;
  ConstIterator end() const;
  Iterator begin();
  Iterator end();

  KMIdentity & newFromScratch( const QString & name );
  KMIdentity & newFromControlCenter( const QString & name );
  KMIdentity & newFromExisting( const KMIdentity & other,
				const QString & name=QString::null );

protected:
  /** The list that will be seen by everyone */
  QValueList<KMIdentity> mIdentities;
  /** The list that will be seen by the config dialog */
  QValueList<KMIdentity> mShadowIdentities;

private:
  void writeConfig() const;
  void readConfig();
  QStringList groupList() const;
  void createDefaultIdentity();
};

#endif // _KMAIL_IDENTITYMANAGER_H_
