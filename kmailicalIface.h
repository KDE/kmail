/*
    This file is part of KMail.
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

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
#ifndef KMAILICALIFACE_H
#define KMAILICALIFACE_H

#include <dcopobject.h>
#include <qstringlist.h>
#include <kurl.h>

// yes, this is this very header - but it tells dcopidl to include it
// in _stub.cpp and _skel.cpp files, to get the definition of the structs.
// ### dcopidlng bug: "" is copied verbatim...
// The kmail/ is so that it can be found by the resources easily
#include <kmail/kmailicalIface.h>

class KMailICalIface : virtual public DCOPObject
{
  K_DCOP

public:
k_dcop:
  struct SubResource {
    //dcopidl barfs on those constructors, but dcopidlng works
    SubResource() {} // for QValueList
    SubResource( const QString& loc, const QString& lab, bool rw, bool ar )
      : location( loc ), label( lab ), writable( rw ), alarmRelevant( ar ) {}
    QString location; // unique
    QString label;    // shown to the user
    bool writable;
    bool alarmRelevant;
  };

  virtual bool addIncidence( const QString& type, const QString& folder,
                             const QString& uid, const QString& ical ) = 0;
  virtual bool deleteIncidence( const QString& type, const QString& folder,
                                const QString& uid ) = 0;
  virtual QStringList incidences( const QString& type,
                                  const QString& folder ) = 0;
  /**
   * Return list of subresources. @p type is
   * Mail, Calendar, Contact, Note, Task or Journal
   */
  virtual QStringList subresources( const QString& type ) = 0;
  virtual bool isWritableFolder( const QString& type,
                                 const QString& resource ) = 0;

  virtual KURL getAttachment( const QString& resource,
                              Q_UINT32 sernum,
                              const QString& filename ) = 0;

  // This saves the iCals/vCards in the entries in the folder.
  // The format in the string list is uid, entry, uid, entry...
  virtual bool update( const QString& type, const QString& folder,
                       const QStringList& entries ) = 0;

  // Update a single entry in the storage layer
  virtual bool update( const QString& type, const QString& folder,
                       const QString& uid, const QString& entry ) = 0;

  /// Update a kolab storage entry. Returns the new mail serial number,
  /// or 0 if something went wrong
  virtual Q_UINT32 update( const QString& resource,
                           Q_UINT32 sernum,
                           const QString& subject,
                           const QString& plainTextBody,
                           const QMap<QCString, QString>& customHeaders,
                           const QStringList& attachmentURLs,
                           const QStringList& attachmentMimetypes,
                           const QStringList& attachmentNames,
                           const QStringList& deletedAttachments ) = 0;

  virtual bool deleteIncidenceKolab( const QString& resource,
                                     Q_UINT32 sernum ) = 0;
  virtual QMap<Q_UINT32, QString> incidencesKolab( const QString& mimetype,
                                                  const QString& resource ) = 0;
  /**
   * Return list of subresources. @p contentsType is
   * Mail, Calendar, Contact, Note, Task or Journal
   */
  virtual QValueList<KMailICalIface::SubResource> subresourcesKolab( const QString& contentsType ) = 0;

  /// The format of the mails containing other contents than actual mail
  /// (like contacts, calendar etc.)
  /// This is currently either ical/vcard, or XML.
  /// The imap resource uses this folder if ical/vcard storage,
  /// the kolab resource uses this folder if xml storage.
  /// For actual mail folders this simply to know which resource handles it
  /// This enum matches the one defined in kmail.kcfg
  enum StorageFormat { StorageIcalVcard, StorageXML };

  /// This bitfield indicates which changes have been made in a folder, at syncing time.
  enum FolderChanges { NoChange = 0, Contents = 1, ACL = 2 };

k_dcop_signals:
  // For vcard/ical type storage (imap resource)
  void incidenceAdded( const QString& type, const QString& folder,
                       const QString& entry );
  void asyncLoadResult( const QStringList& list, const QString& type,
                        const QString& folder );

  // For xml kolab style storage
  void incidenceAdded( const QString& type, const QString& folder,
                       Q_UINT32 sernum, int format, const QString& entry );
  void asyncLoadResult( const QMap<Q_UINT32, QString>, const QString& type,
                        const QString& folder );
  //common
  void incidenceDeleted( const QString& type, const QString& folder,
                         const QString& uid );
  void signalRefresh( const QString& type, const QString& folder );
  void subresourceAdded( const QString& type, const QString& resource );
  void subresourceAdded( const QString& type, const QString& resource,
                         const QString& label, bool writable,
                         bool alarmRelevant );
  void subresourceDeleted( const QString& type, const QString& resource );
};

inline QDataStream& operator<<( QDataStream& str, const KMailICalIface::SubResource& subResource )
{
  str << subResource.location << subResource.label << subResource.writable << subResource.alarmRelevant;
  return str;
}
inline QDataStream& operator>>( QDataStream& str, KMailICalIface::SubResource& subResource )
{
  str >> subResource.location >> subResource.label >> subResource.writable >> subResource.alarmRelevant;
  return str;
}

#endif
