/*
  SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/
#include "kcm_kmail.cpp"
#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KMailMiscConfigFactory, "kmail_config_misc.json", registerPlugin<MiscPage>();)

#include "kcm_kmail_misc.moc"
