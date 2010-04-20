
class Composer;/*
 * This file is part of KMail.
 *
 * Copyright (c) 2010 KDAB
 *
 * Authors: Tobias Koenig <tokoe@kde.org>
 *          Leo Franchi   <lfranchi@kde.org>
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

#ifndef EMAILADDRESSRESOLVEJOB_H
#define EMAILADDRESSRESOLVEJOB_H

#include <kjob.h>

#include <kmime/kmime_message.h>
#include <mailtransport/messagequeuejob.h>

#include <QtCore/QVariant>

namespace Message {
  class Composer;
}

/**
 * @short A job to resolve nicknames, distribution lists and email addresses for queued emails.
 */
class EmailAddressResolveJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new email address resolve job.
     *
     * @param parent The parent object.
     */
    EmailAddressResolveJob( QObject *parent = 0 );

    /**
     * Destroys the email address resolve job.
     */
    ~EmailAddressResolveJob();

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Sets the from address to expand.
     */
    virtual void setFrom( const QString& from );
    
    /**
     * Sets the from address to expand.
     */
    virtual void setTo( const QStringList& from );

    /**
     * Sets the from address to expand.
     */
    virtual void setCc( const QStringList& from );

    /**
     * Sets the from address to expand.
     */
    virtual void setBcc( const QStringList& from );

    /**
     * Returns the expanded From field
     */
    virtual QString expandedFrom() const;

    /**
     * Returns the expanded To field
     */
    virtual QStringList expandedTo() const;

    /**
     * Returns the expanded CC field
     */
    virtual QStringList expandedCc() const;

    /**
     * Returns the expanded Bcc field
     */
    virtual QStringList expandedBcc() const;

  private Q_SLOTS:
    void slotAliasExpansionDone( KJob* );

  private:
    int mJobCount;
    QVariantMap mResultMap;
    QString mFrom;
    QStringList mTo, mCc, mBcc;
};

#endif
