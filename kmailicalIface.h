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
    SubResource( const QString& loc, const QString& lab, bool rw )
      : location( loc ), label( lab ), writable( rw ) {}
    QString location; // unique
    QString label;    // shown to the user
    bool writable;
  };

  /// The format of the mails containing other contents than actual mail
  /// (like contacts, calendar etc.)
  /// This is currently either ical/vcard, or XML.
  /// For actual mail folders this simply to know which resource handles it
  /// This enum matches the one defined in kmail.kcfg
  enum StorageFormat { StorageIcalVcard, StorageXML };

  /// This bitfield indicates which changes have been made in a folder, at syncing time.
  enum FolderChanges { NoChange = 0, Contents = 1, ACL = 2 };

  virtual bool isWritableFolder( const QString& type,
                                 const QString& resource ) = 0;

  virtual KMailICalIface::StorageFormat storageFormat( const QString& resource ) = 0;
  
  virtual KURL getAttachment( const QString& resource,
                              Q_UINT32 sernum,
                              const QString& filename ) = 0;

  /// Update a kolab storage entry. Returns the new mail serial number,
  /// or 0 if something went wrong. Can be used for adding as well.
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

  /// Return the number of mails that need to be looked at by incidencesKolab.
  /// This allows to call incidencesKolab in chunks.
  virtual int incidencesKolabCount( const QString& mimetype,
                                    const QString& resource ) = 0;

  virtual QMap<Q_UINT32, QString> incidencesKolab( const QString& mimetype,
                                                   const QString& resource,
                                                   int startIndex,
                                                   int nbMessages ) = 0;
  /**
   * Return list of subresources. @p contentsType is
   * Mail, Calendar, Contact, Note, Task or Journal
   */
  virtual QValueList<KMailICalIface::SubResource> subresourcesKolab( const QString& contentsType ) = 0;

k_dcop_signals:
  void incidenceAdded( const QString& type, const QString& folder,
                       Q_UINT32 sernum, int format, const QString& entry );
  void asyncLoadResult( const QMap<Q_UINT32, QString>, const QString& type,
                        const QString& folder );
  void incidenceDeleted( const QString& type, const QString& folder,
                         const QString& uid );
  void signalRefresh( const QString& type, const QString& folder );
  void subresourceAdded( const QString& type, const QString& resource,
                         const QString& label );
  void subresourceDeleted( const QString& type, const QString& resource );
};

inline QDataStream& operator<<( QDataStream& str, const KMailICalIface::SubResource& subResource )
{
  str << subResource.location << subResource.label << subResource.writable;
  return str;
}
inline QDataStream& operator>>( QDataStream& str, KMailICalIface::SubResource& subResource )
{
  str >> subResource.location >> subResource.label >> subResource.writable;
  return str;
}

inline QDataStream& operator<<( QDataStream& str, const KMailICalIface::StorageFormat& format  )
{
  Q_UINT32 foo = format;
  str << foo; 
  return str;
}

inline QDataStream& operator>>( QDataStream& str, KMailICalIface::StorageFormat& format  )
{
  Q_UINT32 foo;
  str >> foo; 
  format = ( KMailICalIface::StorageFormat )foo;
  return str;
}


#endif
