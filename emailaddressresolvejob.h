/*
 * This file is part of KMail.
 *
 * Copyright (c) 2010 KDAB
 *
 * Author: Tobias Koenig <tokoe@kde.org>
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
#include <messagecomposer/infopart.h>

#include <QtCore/QVariant>

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
     * @param job The queue job that will put the message in the send queue.
     * @param message The message to send.
     * @param infoPart The information about sender and receiver addresses.
     * @param parent The parent object.
     */
    EmailAddressResolveJob( MailTransport::MessageQueueJob *job, KMime::Message::Ptr message, const Message::InfoPart *infoPart, QObject *parent = 0 );

    /**
     * Destroys the email address resolve job.
     */
    ~EmailAddressResolveJob();

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Sets whether the message shall be @p encrypted.
     */
    void setEncrypted( bool encrypted );

    /**
     * Returns the message queue job this job is working on.
     */
    MailTransport::MessageQueueJob *queueJob() const;

  private Q_SLOTS:
    void slotAliasExpansionDone( KJob* );

  private:
    void finishExpansion();

    MailTransport::MessageQueueJob *mQueueJob;
    KMime::Message::Ptr mMessage;
    const Message::InfoPart *mInfoPart;
    bool mEncrypted;
    int mJobCount;
    QVariantMap mResultMap;
};

#endif
