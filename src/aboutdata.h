/*  -*- c++ -*-
    aboutdata.h

    This file is part of KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2003 Marc Mutz <mutz@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
