/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "potentialphishingemailwarning.h"
#include "potentialphishingdetaildialog.h"
#include <QAction>
#include <KLocalizedString>
#include <QPointer>

PotentialPhishingEmailWarning::PotentialPhishingEmailWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Warning);
    setWordWrap(true);

    setText(i18n("Found potential phishing email addresses <a href=\"phishingdetails\">(details...)</a>"));

    connect(this, &KMessageWidget::linkActivated, this, &PotentialPhishingEmailWarning::slotShowDetails);
    QAction *action = new QAction(i18n("Send Now"), this);
    action->setObjectName(QStringLiteral("sendnow"));
    connect(action, &QAction::triggered, this, &PotentialPhishingEmailWarning::sendNow);
    addAction(action);
}

PotentialPhishingEmailWarning::~PotentialPhishingEmailWarning()
{

}

void PotentialPhishingEmailWarning::slotShowDetails(const QString &link)
{
    if (link == QLatin1String("phishingdetails")) {
        PotentialPhishingDetailDialog dlg(this);
        dlg.fillList(mPotentialPhishingEmails);
        dlg.exec();
    }
}

void PotentialPhishingEmailWarning::setPotentialPhisingEmail(const QStringList &lst)
{
    mPotentialPhishingEmails = lst;
    if (!mPotentialPhishingEmails.isEmpty()) {
        animatedShow();
    }
}
