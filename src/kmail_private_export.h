/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2019-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"

/* Classes which are exported only for unit tests */
#ifdef BUILD_TESTING
#ifndef KMAILTESTS_TESTS_EXPORT
#define KMAILTESTS_TESTS_EXPORT KMAIL_EXPORT
#endif
#else /* not compiling tests */
#define KMAILTESTS_TESTS_EXPORT
#endif

