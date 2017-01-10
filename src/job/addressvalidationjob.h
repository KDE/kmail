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

#ifndef ADDRESSVALIDATIONJOB_H
#define ADDRESSVALIDATIONJOB_H

#include <kjob.h>

class AddressValidationJob : public KJob
{
    Q_OBJECT

public:
    explicit AddressValidationJob(const QString &emailAddresses, QWidget *parentWidget, QObject *parent = nullptr);
    ~AddressValidationJob();

    void start() Q_DECL_OVERRIDE;

    bool isValid() const;

    void setDefaultDomain(const QString &domainName);

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void slotAliasExpansionDone(KJob *))
    //@endcond
};

#endif
