/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "externaleditorwarning.h"

#include <KLocalizedString>

ExternalEditorWarning::ExternalEditorWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(false);
    setMessageType(Information);
    setText(i18n("External editor was started."));
    setWordWrap(true);
}

ExternalEditorWarning::~ExternalEditorWarning() = default;
