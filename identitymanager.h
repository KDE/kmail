/*  -*- c++ -*-
    identitymanager.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
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

  /** @return true if @p addressList contains any of our addresses,
      false otherwise.
      @see #identityForAddress
  **/
  bool thatIsMe( const QString & addressList ) const;

  /** @deprecated
      @return the identity named @p identityName or @ref
      KMIdentity::null if not found.
  **/
  const KMIdentity & identityForName( const QString & identityName ) const;

  /** @return the identity with Unique Object Identifier (UOID) @p
      uoid or @ref KMIdentity::null if not found.
   **/
  const KMIdentity & identityForUoid( uint uoid ) const;

  /** @deprecated
      Convenience method.
  
      @return the identity named @p identityName or the default
      identity if not found.
  **/
  const KMIdentity & identityForNameOrDefault( const QString & identityName ) const;

  /** Convenience menthod.

      @return the identity with Unique Object Identifier (UOID) @p
      uoid or the default identity if not found.
  **/
  const KMIdentity & identityForUoidOrDefault( uint uoid ) const;

  /** @return the default identity */
  const KMIdentity & defaultIdentity() const;

  /** @deprecated
      Sets the identity named @p identityName to be the new default
      identity. As usual, use @ref commit to make this permanent.
      
      @return false if an identity named @p identityName was not found
  **/
  bool setAsDefault( const QString & identityName );

  /** Sets the identity with Unique Object Identifier (UOID) @p uoid
      to be new the default identity. As usual, use @ref commit to
      make this permanent.

      @return false if an identity with UOID @p uoid was not found
  **/
  bool setAsDefault( uint uoid );

  /** @return the identity named @p idenityName. This method returns a
      reference to the identity that can be modified. To let others
      see this change, use @ref commit.
  **/
  KMIdentity & identityForName( const QString & identityName );

  /** @return the identity with Unique Object Identifier (UOID) @p uoid.
      This method returns a reference to the identity that can
      be modified. To let others see this change, use @ref commit.
  **/
  KMIdentity & identityForUoid( uint uoid );

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

signals:
  /** Emitted whenever the identity with Unique Object Identifier
      (UOID) @p uoid changed. Useful for more fine-grained change
      notifications than what is possible with the standard @ref
      changed() signal. */
  void changed( uint uoid );
  /** Emitted whenever the identity @p ident changed. Useful for more
      fine-grained change notifications than what is possible with the
      standard @ref changed() signal. */
  void changed( const KMIdentity & ident );
  /** Emitted on @ref commit() for each deleted identity. At the time
      this signal is emitted, the identity does still exist and can be
      retrieved by @ref identityForUoid() if needed */
  void deleted( uint uoid );
  /** Emitted on @ref commit() for each new identity */
  void added( const KMIdentity & ident );

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

  // returns a new Unique Object Identifier
  int newUoid();
};

#endif // _KMAIL_IDENTITYMANAGER_H_
