/*
  SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/
#include "kcm_kmail.cpp"
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(ComposerPage, "kmail_config_composer.json")

#include "kcm_kmail_composer.moc"
