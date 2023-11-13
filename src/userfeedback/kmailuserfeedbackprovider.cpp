/*
   SPDX-FileCopyrightText: 2020-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmailuserfeedbackprovider.h"
#include "userfeedback/accountinfosource.h"

#include <KF6/KUserFeedback/ApplicationVersionSource>
#include <KF6/KUserFeedback/LocaleInfoSource>
#include <KF6/KUserFeedback/PlatformInfoSource>
#include <KF6/KUserFeedback/QtVersionSource>
#include <KF6/KUserFeedback/ScreenInfoSource>
#include <KF6/KUserFeedback/StartCountSource>
#include <KF6/KUserFeedback/UsageTimeSource>

KMailUserFeedbackProvider::KMailUserFeedbackProvider(QObject *parent)
    : KUserFeedback::Provider(parent)
{
    setProductIdentifier(QStringLiteral("org.kde.kmail"));
    setFeedbackServer(QUrl(QStringLiteral("https://telemetry.kde.org/")));
    setSubmissionInterval(7);
    setApplicationStartsUntilEncouragement(5);
    setEncouragementDelay(30);

    addDataSource(new KUserFeedback::ApplicationVersionSource);
    addDataSource(new KUserFeedback::PlatformInfoSource);
    addDataSource(new KUserFeedback::ScreenInfoSource);
    addDataSource(new KUserFeedback::QtVersionSource);

    addDataSource(new KUserFeedback::StartCountSource);
    addDataSource(new KUserFeedback::UsageTimeSource);

    addDataSource(new KUserFeedback::LocaleInfoSource);
    addDataSource(new AccountInfoSource);
    // addDataSource(new PluginInfoSource);
}

KMailUserFeedbackProvider::~KMailUserFeedbackProvider() = default;

#include "moc_kmailuserfeedbackprovider.cpp"
