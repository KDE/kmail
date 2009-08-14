/*
 * This file is part of KMail.
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KMAIL_ATTACHMENTMODEL_H
#define KMAIL_ATTACHMENTMODEL_H

#include <QtCore/QAbstractItemModel>

#include <libkdepim/attachmentpart.h>

namespace KMail {

/**
  Columns:
  name
  size
  encoding
  mime type
  compress
  encrypt
  sign
*/
class AttachmentModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    enum Column
    {
      NameColumn,
      SizeColumn,
      EncodingColumn,
      MimeTypeColumn,
      CompressColumn,
      EncryptColumn,
      SignColumn,
      LastColumn ///< @internal
    };

    AttachmentModel( QObject *parent );
    ~AttachmentModel();

    /// for the save/discard warning
    bool isModified() const;
    void setModified( bool modified );

    bool isEncryptEnabled() const;
    void setEncryptEnabled( bool enabled );
    bool isSignEnabled() const;
    void setSignEnabled( bool enabled );
    bool isEncryptSelected() const;
    /// sets for all
    void setEncryptSelected( bool selected );
    bool isSignSelected() const;
    /// sets for all
    void setSignSelected( bool selected );

    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    bool addAttachment( KPIM::AttachmentPart::Ptr part );
    bool updateAttachment( KPIM::AttachmentPart::Ptr part );
    bool replaceAttachment( KPIM::AttachmentPart::Ptr oldPart, KPIM::AttachmentPart::Ptr newPart );
    bool removeAttachment( const QModelIndex &index );
    bool removeAttachment( KPIM::AttachmentPart::Ptr part );
    KPIM::AttachmentPart::Ptr attachment( const QModelIndex &index );
    KPIM::AttachmentPart::List attachments();

    Qt::ItemFlags flags( const QModelIndex &index ) const;
    QVariant headerData( int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole ) const;
    QModelIndex index( int row, int column,
                       const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex &index ) const;
    int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    int columnCount( const QModelIndex &parent = QModelIndex() ) const;

  signals:
    void encryptEnabled( bool enabled );
    void signEnabled( bool enabled );
    void attachmentRemoved( KPIM::AttachmentPart::Ptr part );
    void attachmentCompressRequested( KPIM::AttachmentPart::Ptr part, bool compress );

  private:
    class Private;
    friend class Private;
    Private *const d;
};

} // namespace KMail

#endif // KMAIL_ATTACHMENTMODEL_H
