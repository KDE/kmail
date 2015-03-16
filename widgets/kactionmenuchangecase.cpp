/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kactionmenuchangecase.h"
#include <KLocalizedString>
#include <KActionCollection>
#include <KAction>

KActionMenuChangeCase::KActionMenuChangeCase(QObject *parent)
    : KActionMenu(parent)
{
    setText(i18n("Change Case"));
    mUpperCase = new KAction( i18n("Uppercase"), this );
    connect( mUpperCase, SIGNAL(triggered(bool)), this, SIGNAL(upperCase()) );

    mSentenceCase = new KAction( i18n("Sentence case"), this );
    connect( mSentenceCase, SIGNAL(triggered(bool)), this, SIGNAL(sentenceCase()) );

    mLowerCase = new KAction( i18n("Lowercase"), this );
    connect( mLowerCase, SIGNAL(triggered(bool)), this, SIGNAL(lowerCase()) );
    addAction(mUpperCase);
    addAction(mLowerCase);
    addAction(mSentenceCase);
}

KActionMenuChangeCase::~KActionMenuChangeCase()
{

}

KAction *KActionMenuChangeCase::upperCaseAction() const
{
    return mUpperCase;
}

KAction *KActionMenuChangeCase::sentenceCaseAction() const
{
    return mSentenceCase;
}

KAction *KActionMenuChangeCase::lowerCaseAction() const
{
    return mLowerCase;
}

void KActionMenuChangeCase::appendInActionCollection(KActionCollection *ac)
{
    if (ac) {
        ac->addAction( QLatin1String("change_to_uppercase"), mUpperCase );
        ac->addAction( QLatin1String("change_to_sentencecase"), mSentenceCase );
        ac->addAction( QLatin1String("change_to_lowercase"), mLowerCase );
    }
}
