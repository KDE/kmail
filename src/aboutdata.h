/*  -*- c++ -*-
    aboutdata.h

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2003 Marc Mutz <mutz@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include "kmail_export.h"
#include <KAboutData>

namespace KMail
{
class KMAIL_EXPORT AboutData : public KAboutData
{
public:
    AboutData();
    ~AboutData();
};
} // namespace KMail

