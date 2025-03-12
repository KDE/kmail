/*
   SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sendlaterinfoconfigwidget.h"
#include "sendlaterremovemessagejob.h"

#include "kmail-version.h"
#include "sendlaterconfigurewidget.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QApplication>
#include <QIcon>
#include <QWindow>
using namespace Qt::Literals::StringLiterals;
namespace
{
static const char myConfigureSendLaterConfigureDialogGroupName[] = "SendLaterConfigureDialog";
}

SendLaterInfoConfigWidget::SendLaterInfoConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mWidget(new SendLaterWidget(parent))
{
    parent->layout()->addWidget(mWidget);
    KAboutData aboutData = KAboutData(QStringLiteral("sendlateragent"),
                                      i18n("Send Later Agent"),
                                      QStringLiteral(KDEPIM_VERSION),
                                      i18n("Send emails later agent."),
                                      KAboutLicense::GPL_V2,
                                      i18n("Copyright (C) 2013-%1 Laurent Montel", QStringLiteral("2025")));

    aboutData.addAuthor(i18nc("@info:credit", "Laurent Montel"), i18n("Maintainer"), QStringLiteral("montel@kde.org"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    setKAboutData(aboutData);

    aboutData.setProductName(QByteArrayLiteral("Akonadi/SendLaterAgent"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
}

SendLaterInfoConfigWidget::~SendLaterInfoConfigWidget() = default;

QList<Akonadi::Item::Id> SendLaterInfoConfigWidget::messagesToRemove() const
{
    return mWidget->messagesToRemove();
}

void SendLaterInfoConfigWidget::load()
{
    mWidget->load();
}

bool SendLaterInfoConfigWidget::save() const
{
    const QList<Akonadi::Item::Id> listMessage = mWidget->messagesToRemove();
    if (!listMessage.isEmpty()) {
        // Will delete in specific job when done.
        auto sendlaterremovejob = new SendLaterRemoveMessageJob(listMessage);
        sendlaterremovejob->start();
    }
    return mWidget->save();
}

void SendLaterInfoConfigWidget::slotNeedToReloadConfig()
{
    mWidget->needToReload();
}

QSize SendLaterInfoConfigWidget::restoreDialogSize() const
{
    auto group = config()->group(QLatin1StringView(myConfigureSendLaterConfigureDialogGroupName));
    const QSize size = group.readEntry("Size", QSize(800, 600));
    return size;
}

void SendLaterInfoConfigWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group(QLatin1StringView(myConfigureSendLaterConfigureDialogGroupName));
    group.writeEntry("Size", size);
}

#include "moc_sendlaterinfoconfigwidget.cpp"
