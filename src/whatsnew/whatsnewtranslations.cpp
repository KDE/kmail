/*
   SPDX-FileCopyrightText: 2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "whatsnewtranslations.h"

WhatsNewTranslations::WhatsNewTranslations() = default;

WhatsNewTranslations::~WhatsNewTranslations() = default;

// Use by newFeaturesMD5
QList<KLazyLocalizedString> WhatsNewTranslations::lastNewFeatures() const
{
    const QList<KLazyLocalizedString> info{
        kli18n("HTML messages are shown with their intended colors by default. To override this, uncheck the \"Appearance->Colors->Do not change color from "
               "original HTML mail\" option")};
    return info;
}

QList<TextAddonsWidgets::WhatsNewInfo> WhatsNewTranslations::createWhatsNewInfo() const
{
    QList<TextAddonsWidgets::WhatsNewInfo> listInfo;
    {
        TextAddonsWidgets::WhatsNewInfo info;
        info.setNewFeatures({i18n("Add new whatsnew widget."), i18n("New plugin: show collection size.")});
        info.setVersion(QStringLiteral("6.5.0"));
        listInfo.append(std::move(info));
    }
    {
        TextAddonsWidgets::WhatsNewInfo info;
        QStringList lst;
        for (const KLazyLocalizedString &l : lastNewFeatures()) {
            lst += l.toString();
        }
        info.setNewFeatures({i18n("Allow to export text in autogenerate editor plugins.")});
        info.setBugFixings({i18n("Fix text to speech support (enqueue messages)")});
        info.setVersion(QStringLiteral("6.6.0"));
        listInfo.append(std::move(info));
    }
    {
        TextAddonsWidgets::WhatsNewInfo info;
        QStringList lst;
        for (const KLazyLocalizedString &l : lastNewFeatures()) {
            lst += l.toString();
        }
        info.setNewFeatures(lst);
        info.setBugFixings({});
        info.setVersion(QStringLiteral("6.7.0"));
        listInfo.append(std::move(info));
    }
    return listInfo;
}
