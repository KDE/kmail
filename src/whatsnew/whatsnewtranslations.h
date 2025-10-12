/*
   SPDX-FileCopyrightText: 2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once
#include "config-kmail.h"
#include "kmail_export.h"
#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
#include <TextAddonsWidgets/WhatsNewTranslationsBase>
#else
#include <PimCommon/WhatsNewTranslationsBase>
#endif

#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
class KMAIL_EXPORT WhatsNewTranslations : public TextAddonsWidgets::WhatsNewTranslationsBase
#else
class KMAIL_EXPORT WhatsNewTranslations : public PimCommon::WhatsNewTranslationsBase
#endif
{
public:
    WhatsNewTranslations();
    ~WhatsNewTranslations() override;
#if HAVE_TEXTUTILS_HAS_WHATSNEW_SUPPORT
    [[nodiscard]] QList<TextAddonsWidgets::WhatsNewInfo> createWhatsNewInfo() const override;
#else
    [[nodiscard]] QList<PimCommon::WhatsNewInfo> createWhatsNewInfo() const override;
#endif
    [[nodiscard]] QList<KLazyLocalizedString> lastNewFeatures() const override;
};
