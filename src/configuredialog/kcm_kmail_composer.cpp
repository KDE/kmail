/*
  SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/
#include "kcm_kmail.cpp"
#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KMailComposerConfigFactory, "kmail_config_composer.json", registerPlugin<ComposerPage>();)

#include "kcm_kmail_composer.moc"
