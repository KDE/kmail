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
    const QList<KLazyLocalizedString> info{kli18n("Allow to export text in autogenerate editor plugins.")};
    return info;
}

#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
QList<TextAddonsWidgets::WhatsNewInfo> WhatsNewTranslations::createWhatsNewInfo() const
#else
QList<PimCommon::WhatsNewInfo> WhatsNewTranslations::createWhatsNewInfo() const
#endif
{
#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
    QList<TextAddonsWidgets::WhatsNewInfo> listInfo;
#else
    QList<PimCommon::WhatsNewInfo> listInfo;
#endif
    {
#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
        TextAddonsWidgets::WhatsNewInfo info;
#else
        PimCommon::WhatsNewInfo info;
#endif
        info.setNewFeatures({i18n("Add new whatsnew widget."), i18n("New plugin: show collection size.")});
        info.setVersion(QStringLiteral("6.5.0"));
        listInfo.append(std::move(info));
    }
    {
#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
        TextAddonsWidgets::WhatsNewInfo info;
#else
        PimCommon::WhatsNewInfo info;
#endif
        QStringList lst;
        for (const KLazyLocalizedString &l : lastNewFeatures()) {
            lst += l.toString();
        }
        info.setNewFeatures(lst);
        info.setVersion(QStringLiteral("6.6.0"));
        listInfo.append(std::move(info));
    }
    return listInfo;
}
