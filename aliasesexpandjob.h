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

#ifndef ALIASESEXPANDJOB_H
#define ALIASESEXPANDJOB_H

#include <kjob.h>

#include <QtCore/QMap>
#include <QtCore/QStringList>

/**
 * @short A job to expand aliases to email addresses.
 *
 * Expands aliases (distribution lists and nick names) and appends a
 * domain part to all email addresses which are missing the domain part.
 */
class AliasesExpandJob : public KJob
{
  Q_OBJECT

  public:
    /**
     * Creates a new aliases expand job.
     *
     * @param recipients The comma separated list of recipient.
     * @param defaultDomain The default domain that shall be used when expanding aliases.
     * @param parent The parent object.
     */
    AliasesExpandJob( const QString &recipients, const QString &defaultDomain, QObject *parent = 0 );

    /**
     * Destroys the aliases expand job.
     */
    ~AliasesExpandJob();

    /**
     * Starts the job.
     */
    virtual void start();

    /**
     * Returns the expanded email addresses.
     */
    QString addresses() const;

    /**
     * Returns the list of distribution lists that resolved to an empty member list.
     */
    QStringList emptyDistributionLists() const;

  private Q_SLOTS:
    void slotDistributionListExpansionDone( KJob* );
    void slotNicknameExpansionDone( KJob* );

  private:
    void finishExpansion();

    QStringList mRecipients;
    QString mDefaultDomain;

    QString mEmailAddresses;
    QStringList mEmptyDistributionLists;

    uint mDistributionListExpansionJobs;
    uint mNicknameExpansionJobs;

    struct DistributionListExpansionResult
    {
      QString addresses;
      bool isEmpty;
    };
    QMap<QString, DistributionListExpansionResult> mDistListExpansionResults;
    QMap<QString, QString> mNicknameExpansionResults;
};

#endif
