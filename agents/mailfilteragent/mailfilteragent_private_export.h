/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "mailfilteragentprivate_export.h"

/* Classes which are exported only for unit tests */
#ifdef BUILD_TESTING
#ifndef MAILFILTERAGENTPRIVATE_TESTS_EXPORT
#define MAILFILTERAGENTPRIVATE_TESTS_EXPORT MAILFILTERAGENTPRIVATE_EXPORT
#endif
#else /* not compiling tests */
#define MAILFILTERAGENTPRIVATE_TESTS_EXPORT
#endif
