/*
  SPDX-FileCopyrightText: 2021-2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "configureappearancepage.h"
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(AppearancePage, "kmail_config_appearance.json")

#include "kcm_kmail_appearance.moc"
